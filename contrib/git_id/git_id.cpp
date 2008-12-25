#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <assert.h>
#include <set>
#include "../../src/framework/Platform/CompilerDefs.h"

#if PLATFORM == PLATFORM_WINDOWS
#include <direct.h>
#define popen _popen
#define pclose _pclose
#define snprintf _snprintf
#pragma warning (disable:4996)
#else
#include <unistd.h>
#endif

// max string sizes

#define MAX_REMOTE 256
#define MAX_MSG 16384
#define MAX_PATH 2048
#define MAX_BUF 2048
#define MAX_CMD 2048
#define MAX_HASH 256
#define MAX_DB 256

// config

#define NUM_REMOTES 2
#define NUM_DATABASES 3

char remotes[NUM_REMOTES][MAX_REMOTE] = {
    "git@github.com:mangos/mangos.git",
    "git://github.com/mangos/mangos.git"        // used for fetch if present
};

char remote_branch[MAX_REMOTE] = "master";
char rev_file[MAX_PATH] = "src/shared/revision_nr.h";
char sql_update_dir[MAX_PATH] = "sql/updates";

char databases[NUM_DATABASES][MAX_DB] = {
    "characters",
    "mangos",
    "realmd"
};

char db_version_table[NUM_DATABASES][MAX_DB] = {
    "character_db_version",
    "db_version",
    "realmd_db_version",
};

bool allow_replace = false;
bool local = false;
bool do_fetch = false;
bool do_sql = false;

// aux

char origins[NUM_REMOTES][MAX_REMOTE];
int rev;
int last_sql_rev[NUM_DATABASES] = {0,0,0};

char head_message[MAX_MSG];
char path_prefix[MAX_PATH] = "";
char buffer[MAX_BUF];
char cmd[MAX_CMD];
char origin_hash[MAX_HASH];
char last_sql_update[NUM_DATABASES][MAX_PATH];

std::set<std::string> new_sql_updates;

FILE *cmd_pipe;

bool find_path()
{
    printf("+ finding path\n");
    char cur_path[MAX_PATH], *ptr;
    getcwd(cur_path, MAX_PATH);
    int len = strnlen(cur_path, MAX_PATH);
    
    if(cur_path[len-1] == '/' || cur_path[len-1] == '\\')
    {
        // we're in root, don't bother
        return false;
    }

    // don't count the root
    int count_fwd = 0, count_back = 0;
    for(ptr = cur_path-1; ptr = strchr(ptr+1, '/'); count_fwd++);
    for(ptr = cur_path-1; ptr = strchr(ptr+1, '\\'); count_back++);
    int count = std::max(count_fwd, count_back);

    char path[MAX_PATH];
    for(int i = 0; i < count; i++)
    {
        snprintf(path, MAX_PATH, "%s.git", path_prefix);
        if(0 == chdir(path))
        {
            chdir(cur_path);
            return true;
        }
        strncat(path_prefix, "../", MAX_PATH);
    }

    return false;
}

bool find_origin()
{
    printf("+ finding origin\n");
    if( (cmd_pipe = popen( "git remote -v", "r" )) == NULL )
        return false;

    bool ret = false;
    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        char name[256], remote[MAX_REMOTE];
        sscanf(buffer, "%s %s", name, remote);
        for(int i = 0; i < NUM_REMOTES; i++)
        {
            if(strcmp(remote, remotes[i]) == 0)
            {
                strncpy(origins[i], name, MAX_REMOTE);
                ret = true;
            }
        }
    }
    pclose(cmd_pipe);
    return ret;
}

bool fetch_origin()
{
    printf("+ fetching origin\n");
    // use the public clone url if present because the private may require a password
    snprintf(cmd, MAX_CMD, "git fetch %s %s", (origins[1][0] ? origins[1] : origins[0]), remote_branch);
    int ret = system(cmd);
    return true;
}

bool check_fwd()
{
    printf("+ checking fast forward\n");
    snprintf(cmd, MAX_CMD, "git log -n 1 --pretty=\"format:%%H\" %s/%s", (origins[1][0] ? origins[1] : origins[0]), remote_branch);
    if( (cmd_pipe = popen( cmd, "r" )) == NULL )
        return false;

    if(!fgets(buffer, MAX_BUF, cmd_pipe)) return false;
    strncpy(origin_hash, buffer, MAX_HASH);
    pclose(cmd_pipe);

    if( (cmd_pipe = popen( "git log --pretty=\"format:%H\"", "r" )) == NULL )
        return false;

    bool found = false;
    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        buffer[strnlen(buffer, MAX_BUF) - 1] = '\0';
        if(strncmp(origin_hash, buffer, MAX_BUF) == 0)
        {
            found = true;
            break;
        }
    }
    pclose(cmd_pipe);

    if(!found) printf("WARNING: non-fastforward, use rebase!\n");
    return true;
}

int get_rev(const char *from_msg)
{
    // accept only the rev number format, not the sql update format
    char nr_str[256];
    if(sscanf(from_msg, "[%[0123456789]]", nr_str) != 1) return 0;
    if(from_msg[strlen(nr_str)+1] != ']') return 0;
    return atoi(nr_str);
}

bool find_rev()
{
    printf("+ finding next revision number\n");
    // find the highest rev number on either of the remotes
    for(int i = 0; i < NUM_REMOTES; i++)
    {
        if(!local && !origins[i][0]) continue;

        if(local) snprintf(cmd, MAX_CMD, "git log HEAD --pretty=\"format:%%s\"");
        else sprintf(cmd, "git log %s/%s --pretty=\"format:%%s\"", origins[i], remote_branch);
        if( (cmd_pipe = popen( cmd, "r" )) == NULL )
            continue;

        int nr;
        while(fgets(buffer, MAX_BUF, cmd_pipe))
        {
            nr = get_rev(buffer);
            if(nr >= rev)
                rev = nr+1;
        }
        pclose(cmd_pipe);
    }

    if(rev > 0) printf("Found [%d].\n", rev);

    return rev > 0;
}

std::string generateHeader(char const* rev_str)
{
    std::ostringstream newData;
    newData << "#ifndef __REVISION_NR_H__" << std::endl;
    newData << "#define __REVISION_NR_H__"  << std::endl;
    newData << " #define REVISION_NR \"" << rev_str << "\"" << std::endl;
    newData << "#endif // __REVISION_NR_H__" << std::endl;
    return newData.str();
}

bool write_rev()
{
    printf("+ writing revision_nr.h\n");
    char rev_str[256];
    sprintf(rev_str, "%d", rev);
    std::string header = generateHeader(rev_str);

    char prefixed_file[MAX_PATH];
    snprintf(prefixed_file, MAX_PATH, "%s%s", path_prefix, rev_file);

    if(FILE* OutputFile = fopen(prefixed_file, "wb"))
    {
        fprintf(OutputFile,"%s", header.c_str());
        fclose(OutputFile);
        return true;
    }

    return false;
}

bool find_head_msg()
{
    printf("+ finding last message on HEAD\n");
    if( (cmd_pipe = popen( "git log -n 1 --pretty=\"format:%s%n%n%b\"", "r" )) == NULL )
        return false;

    int poz = 0;
    while(poz < 16384-1 && EOF != (head_message[poz++] = fgetc(cmd_pipe)));
    head_message[poz-1] = '\0';

    pclose(cmd_pipe);

    if(int head_rev = get_rev(head_message))
    {
        if(!allow_replace)
        {
            printf("Last commit on HEAD is [%d]. Use -r to replace it with [%d].\n", head_rev, rev);
            return false;
        }

        // skip the rev number in the commit
        char *p = strchr(head_message, ']'), *q = head_message;
        assert(p && *(p+1));
        p+=2;
        while(*p) *q = *p, p++, q++;
        *q = 0;
        return true;
    }

    return true;
}

bool amend_commit()
{
    printf("+ amending last commit\n");
    snprintf(cmd, MAX_CMD, "git commit --amend -F- %s%s", path_prefix, rev_file);
    if( (cmd_pipe = popen( cmd, "w" )) == NULL )
        return false;

    fprintf(cmd_pipe, "[%d] %s", rev, head_message);
    pclose(cmd_pipe);
    
    return true;
}

bool find_sql_updates()
{
    printf("+ finding new sql updates on HEAD\n");
    // add all updates from HEAD
    snprintf(cmd, MAX_CMD, "git show HEAD:%s", sql_update_dir);
    if( (cmd_pipe = popen( cmd, "r" )) == NULL )
        return false;

    // skip first two lines
    if(!fgets(buffer, MAX_BUF, cmd_pipe)) { pclose(cmd_pipe); return false; }
    if(!fgets(buffer, MAX_BUF, cmd_pipe)) { pclose(cmd_pipe); return false; }

    int nr, cur_rev, i;
    char db[MAX_BUF], table[MAX_BUF];

    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        buffer[strnlen(buffer, MAX_BUF) - 1] = '\0';
        if(sscanf(buffer, "%d_%d_%[^_]_%[^.].sql", &cur_rev, &nr, db, table) == 4 ||
            sscanf(buffer, "%d_%d_%[^.].sql", &cur_rev, &nr, db) == 3)
        {
            // find the update with the highest rev for each database
            // (will be the required version for the new update)
            // new updates should not have a rev number already
            for(i = 0; i < NUM_DATABASES; i++)
                if(cur_rev > last_sql_rev[i] &&
                    strncmp(db, databases[i], MAX_DB) == 0)
                    break;

            if(i < NUM_DATABASES)
            {
                last_sql_rev[i] = cur_rev;
                strncpy(last_sql_update[i], buffer, MAX_PATH);
            }
        }
        else if(sscanf(buffer, "%d_%[^_]_%[^.].sql", &nr, db, table) == 3 ||
            sscanf(buffer, "%d_%[^.].sql", &nr, db) == 2)
        {
            for(i = 0; i < NUM_DATABASES; i++)
                if(strncmp(db, databases[i], MAX_DB) == 0) break;
            if(i == NUM_DATABASES)
                printf("WARNING: incorrect database name for sql update %s\n", buffer);
            else
                new_sql_updates.insert(buffer);
        } 
    }

    pclose(cmd_pipe);

    // remove updates from the last commit also found on origin
    snprintf(cmd, MAX_CMD, "git show %s:%s", origin_hash, sql_update_dir);
    if( (cmd_pipe = popen( cmd, "r" )) == NULL )
        return false;

    // skip first two lines
    if(!fgets(buffer, MAX_BUF, cmd_pipe)) { pclose(cmd_pipe); return false; }
    if(!fgets(buffer, MAX_BUF, cmd_pipe)) { pclose(cmd_pipe); return false; }

    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        buffer[strnlen(buffer, MAX_BUF) - 1] = '\0';
        if(sscanf(buffer, "%d_%[^.].sql", &nr, db) == 2)
            new_sql_updates.erase(buffer);
    }

    pclose(cmd_pipe);

    if(!new_sql_updates.empty())
    {
        for(std::set<std::string>::iterator itr = new_sql_updates.begin(); itr != new_sql_updates.end(); ++itr)
            printf("%s\n", itr->c_str());
    }
    else
        printf("WARNING: no new sql updates found.\n");

    return true;
}

bool convert_sql_update(const char *src_file, const char *dst_file, const char *dst_name)
{
    FILE * fin = fopen( src_file, "r" );
    if(!fin) return false;
    FILE * fout = fopen( dst_file, "w" );
    if(!fout) { fclose(fin); return false; }

    int cur_rev, nr, i;
    char db[MAX_PATH], table[MAX_PATH];
    if(sscanf(dst_name, "%d_%d_%[^_]_%[^.].sql", &cur_rev, &nr, db, table) != 4 &&
        sscanf(dst_name, "%d_%d_%[^.].sql", &cur_rev, &nr, db) != 3)
        return false;

    for(i = 0; i < NUM_DATABASES; i++)
        if(strncmp(db, databases[i], MAX_DB) == 0) break;
    if(i == NUM_DATABASES) return false;

    fprintf(fout, "ALTER TABLE %s CHANGE COLUMN required_%s required_%s bit;\n\n",
        db_version_table[i], last_sql_update[i], dst_name);

    char c;
    while( (c = getc(fin)) != EOF )
        putc(c, fout);

    fclose(fin);
    fclose(fout);

    return true;
}

bool convert_sql_updates()
{
    if(new_sql_updates.empty()) return true;

    printf("+ converting sql updates\n");

    for(std::set<std::string>::iterator itr = new_sql_updates.begin(); itr != new_sql_updates.end(); ++itr)
    {
        char src_file[MAX_PATH], dst_file[MAX_PATH], dst_name[MAX_PATH];
        snprintf(src_file, MAX_PATH, "%s%s/%s", path_prefix, sql_update_dir, itr->c_str());
        snprintf(dst_name, MAX_PATH, "%d_%s", rev, itr->c_str());
        snprintf(dst_file, MAX_PATH, "%s%s/%s", path_prefix, sql_update_dir, dst_name);
        if(!convert_sql_update(src_file, dst_file, dst_name)) return false;
        snprintf(cmd, MAX_CMD, "git add %s", dst_file);
        system(cmd);
        snprintf(cmd, MAX_CMD, "git rm %s", src_file);
        system(cmd);
    }

    return true;
}

bool generate_sql_makefile()
{
    if(new_sql_updates.empty()) return true;

    // find all files in the update dir
    snprintf(cmd, MAX_CMD, "git show HEAD:%s", sql_update_dir);
    if( (cmd_pipe = popen( cmd, "r" )) == NULL )
        return false;

    // skip first two lines
    if(!fgets(buffer, MAX_BUF, cmd_pipe)) { pclose(cmd_pipe); return false; }
    if(!fgets(buffer, MAX_BUF, cmd_pipe)) { pclose(cmd_pipe); return false; }

    char newname[MAX_PATH];
    std::set<std::string> file_list;

    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        buffer[strnlen(buffer, MAX_BUF) - 1] = '\0';
        if(buffer[strnlen(buffer, MAX_BUF) - 1] != '/' &&
            strncmp(buffer, "Makefile.am", MAX_BUF) != 0)
        {
            if(new_sql_updates.find(buffer) != new_sql_updates.end())
            {
                snprintf(newname, MAX_PATH, "%d_%s", rev, buffer);
                file_list.insert(newname);
            }
            else
                file_list.insert(buffer);
        }
    }

    pclose(cmd_pipe);

    // write the makefile
    char file_name[MAX_PATH];
    snprintf(file_name, MAX_PATH, "%s%s/Makefile.am", path_prefix, sql_update_dir);
    FILE *fout = fopen(file_name, "w");
    if(!fout) { pclose(cmd_pipe); return false; }

    fprintf(fout,
        "# Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>\n"
        "#\n"
        "# This program is free software; you can redistribute it and/or modify\n"
        "# it under the terms of the GNU General Public License as published by\n"
        "# the Free Software Foundation; either version 2 of the License, or\n"
        "# (at your option) any later version.\n"
        "#\n"
        "# This program is distributed in the hope that it will be useful,\n"
        "# but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "# GNU General Public License for more details.\n"
        "#\n"
        "# You should have received a copy of the GNU General Public License\n"
        "# along with this program; if not, write to the Free Software\n"
        "# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
        "\n"
        "## Process this file with automake to produce Makefile.in\n"
        "\n"
        "## Sub-directories to parse\n"
        "\n"
        "## Change installation location\n"
        "#  datadir = mangos/%s\n"
        "pkgdatadir = $(datadir)/mangos/%s\n"
        "\n"
        "## Files to be installed\n"
        "#  Install basic SQL files to datadir\n"
        "pkgdata_DATA = \\\n",
        sql_update_dir, sql_update_dir
    );

    for(std::set<std::string>::iterator itr = file_list.begin(); itr != file_list.end(); ++itr)
        fprintf(fout, "\t%s \\\n", itr->c_str());

    fprintf(fout, 
        "\n## Additional files to include when running 'make dist'\n"
        "#  SQL update files, to upgrade database schema from older revisions\n"
        "EXTRA_DIST = \\\n"
    );

    for(std::set<std::string>::iterator itr = file_list.begin(); itr != file_list.end(); ++itr)
        fprintf(fout, "\t%s \\\n", itr->c_str());

    fclose(fout);

    return true;
}

#define DO(cmd) if(!cmd) { printf("FAILED\n"); return 1; }

int main(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        if(argv[i] == NULL) continue;
        if(strncmp(argv[i], "-r", 2) == 0 || strncmp(argv[i], "--replace", 9) == 0)
            allow_replace = true;
        else if(strncmp(argv[i], "-l", 2) == 0 || strncmp(argv[i], "--local", 7) == 0)
            local = true;
        else if(strncmp(argv[i], "-f", 2) == 0 || strncmp(argv[i], "--fetch", 7) == 0)
            do_fetch = true;
        else if(strncmp(argv[i], "-s", 2) == 0 || strncmp(argv[i], "--sql", 5) == 0)
            do_sql = true;
        else if(strncmp(argv[i], "--branch=", 9) == 0)
            snprintf(remote_branch, MAX_REMOTE, "%s", argv[i] + 9);
        else if(strncmp(argv[i], "-h", 2) == 0 || strncmp(argv[i], "--help", 6) == 0)
        {
            printf("Usage: git_id [OPTION]\n");
            printf("Generates a new rev number and updates revision_nr.h and the commit message.\n");
            printf("Should be used just before push.\n");
            printf("   -h, --help     show the usage\n");
            printf("   -r, --replace  replace the rev number if it was already applied to the last\n");
            printf("                  commit\n");
            printf("   -l, --local    search for the highest rev number on HEAD\n");
            printf("   -f, --fetch    fetch from origin before searching for the new rev\n");
            printf("   -s, --sql      ");
            printf("       --branch=BRANCH");
            return 0;
        }
    }

    DO( find_path()     );
    if(!local)
    {
        DO( find_origin()   );
        if(do_fetch)
            DO( fetch_origin()  );
        DO( check_fwd()     );
    }
    DO( find_rev()      );
    DO( find_head_msg() );
    if(do_sql)
        DO( find_sql_updates() );
    DO( write_rev()     );
    if(do_sql)
    {
        DO( convert_sql_updates()    );
        DO( generate_sql_makefile() );
    }
    DO( amend_commit()  );

    return 0;
}
