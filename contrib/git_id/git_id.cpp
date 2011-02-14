/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <assert.h>
#include <set>
#include <list>
#include <sstream>

#ifdef WIN32
#include <direct.h>
#define popen _popen
#define pclose _pclose
#define snprintf _snprintf
#define putenv _putenv
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
char rev_nr_file[MAX_PATH] = "src/shared/revision_nr.h";
char rev_sql_file[MAX_PATH] = "src/shared/revision_sql.h";
char sql_update_dir[MAX_PATH] = "sql/updates";
char new_index_file[MAX_PATH] = ".git/git_id_index";

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

char db_sql_file[NUM_DATABASES][MAX_PATH] = {
    "sql/characters.sql",
    "sql/mangos.sql",
    "sql/realmd.sql"
};

char db_sql_rev_field[NUM_DATABASES][MAX_PATH] = {
    "REVISION_DB_CHARACTERS",
    "REVISION_DB_MANGOS",
    "REVISION_DB_REALMD"
};

bool allow_replace = false;
bool local = false;
bool do_fetch = false;
bool do_sql = false;
bool use_new_index = true;
bool generate_makefile = false;                             // not need for cmake build systems
// aux

char origins[NUM_REMOTES][MAX_REMOTE];
int rev;
int last_sql_rev[NUM_DATABASES] = {0,0,0};
int last_sql_nr[NUM_DATABASES] = {0,0,0};

char head_message[MAX_MSG];
char path_prefix[MAX_PATH] = "";
char base_path[MAX_PATH];
char buffer[MAX_BUF];
char cmd[MAX_CMD];
char origin_hash[MAX_HASH];
char last_sql_update[NUM_DATABASES][MAX_PATH];
char old_index_cmd[MAX_CMD];
char new_index_cmd[MAX_CMD];

std::set<std::string> new_sql_updates;

FILE *cmd_pipe;

bool find_path()
{
    printf("+ finding path\n");
    char *ptr;
    char cur_path[MAX_PATH];
    getcwd(cur_path, MAX_PATH);
    size_t len = strlen(cur_path);
    strncpy(base_path, cur_path, len+1);

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

        ptr = strrchr(base_path, '\\');
        if(ptr) *ptr = '\0';
        else
        {
            ptr = strrchr(base_path, '/');
            if(ptr) *ptr = '\0';
        }
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
        buffer[strlen(buffer) - 1] = '\0';
        if(strncmp(origin_hash, buffer, MAX_BUF) == 0)
        {
            found = true;
            break;
        }
    }
    pclose(cmd_pipe);

    if(!found)
    {
        // with fetch you still get the latest rev, you just rebase afterwards and push
        // without it you may not get the right rev
        if(do_fetch) printf("WARNING: non-fastforward, use rebase!\n");
        else { printf("ERROR: non-fastforward, use rebase!\n"); return false; }
    }
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

std::string generateNrHeader(char const* rev_str)
{
    std::ostringstream newData;
    newData << "#ifndef __REVISION_NR_H__" << std::endl;
    newData << "#define __REVISION_NR_H__"  << std::endl;
    newData << " #define REVISION_NR \"" << rev_str << "\"" << std::endl;
    newData << "#endif // __REVISION_NR_H__" << std::endl;
    return newData.str();
}

std::string generateSqlHeader()
{
    std::ostringstream newData;
    newData << "#ifndef __REVISION_SQL_H__" << std::endl;
    newData << "#define __REVISION_SQL_H__"  << std::endl;
    for(int i = 0; i < NUM_DATABASES; ++i)
        newData << " #define " << db_sql_rev_field[i] << " \"required_" << last_sql_update[i] << "\"" << std::endl;
    newData << "#endif // __REVISION_SQL_H__" << std::endl;
    return newData.str();
}

void system_switch_index(const char *cmd)
{
    // do the command for the original index and then for the new index
    // both need to be updated with the changes before commit
    // but the new index will contains only the desired changes
    // while the old may contain others
    system(cmd);
    if(!use_new_index) return;
    if(putenv(new_index_cmd) != 0) return;
    system(cmd);
    if(putenv(old_index_cmd) != 0) return;
}

bool write_rev_nr()
{
    printf("+ writing revision_nr.h\n");
    char rev_str[256];
    sprintf(rev_str, "%d", rev);
    std::string header = generateNrHeader(rev_str);

    char prefixed_file[MAX_PATH];
    snprintf(prefixed_file, MAX_PATH, "%s%s", path_prefix, rev_nr_file);

    if(FILE* OutputFile = fopen(prefixed_file, "wb"))
    {
        fprintf(OutputFile,"%s", header.c_str());
        fclose(OutputFile);

        // add the file to both indices, to be committed later
        snprintf(cmd, MAX_CMD, "git add %s", prefixed_file);
        system_switch_index(cmd);

        return true;
    }

    return false;
}

bool write_rev_sql()
{
    if(new_sql_updates.empty()) return true;
    printf("+ writing revision_sql.h\n");
    std::string header = generateSqlHeader();

    char prefixed_file[MAX_PATH];
    snprintf(prefixed_file, MAX_PATH, "%s%s", path_prefix, rev_sql_file);

    if(FILE* OutputFile = fopen(prefixed_file, "wb"))
    {
        fprintf(OutputFile,"%s", header.c_str());
        fclose(OutputFile);

        // add the file to both indices, to be committed later
        snprintf(cmd, MAX_CMD, "git add %s", prefixed_file);
        system_switch_index(cmd);

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

    // commit the contents of the (new) index
    if(use_new_index && putenv(new_index_cmd) != 0) return false;
    snprintf(cmd, MAX_CMD, "git commit --amend -F-");
    if( (cmd_pipe = popen( cmd, "w" )) == NULL )
        return false;

    fprintf(cmd_pipe, "[%d] %s", rev, head_message);
    pclose(cmd_pipe);
    if(use_new_index && putenv(old_index_cmd) != 0) return false;

    return true;
}

struct sql_update_info
{
    int rev;
    int nr;
    int db_idx;
    char db[MAX_BUF];
    char table[MAX_BUF];
    bool has_table;
};

bool get_sql_update_info(const char *buffer, sql_update_info &info)
{
    info.table[0] = '\0';
    int dummy[3];
    if(sscanf(buffer, "%d_%d_%d", &dummy[0], &dummy[1], &dummy[2]) == 3)
        return false;

    if(sscanf(buffer, "%d_%d_%[^_]_%[^.].sql", &info.rev, &info.nr, info.db, info.table) != 4 &&
        sscanf(buffer, "%d_%d_%[^.].sql", &info.rev, &info.nr, info.db) != 3)
    {
        info.rev = 0;       // this may be set by the first scans, even if they fail
        if(sscanf(buffer, "%d_%[^_]_%[^.].sql", &info.nr, info.db, info.table) != 3 &&
            sscanf(buffer, "%d_%[^.].sql", &info.nr, info.db) != 2)
            return false;
    }

    for(info.db_idx = 0; info.db_idx < NUM_DATABASES; info.db_idx++)
        if(strncmp(info.db, databases[info.db_idx], MAX_DB) == 0) break;
    info.has_table = (info.table[0] != '\0');
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

    sql_update_info info;

    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        buffer[strlen(buffer) - 1] = '\0';
        if(!get_sql_update_info(buffer, info)) continue;

        if(info.db_idx == NUM_DATABASES)
        {
             if(info.rev > 0) printf("WARNING: incorrect database name for sql update %s\n", buffer);
             continue;
        }

        new_sql_updates.insert(buffer);
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
        buffer[strlen(buffer) - 1] = '\0';
        if(!get_sql_update_info(buffer, info)) continue;

        // find the old update with the highest rev for each database
        // (will be the required version for the new update)
        std::set<std::string>::iterator itr = new_sql_updates.find(buffer);
        if(itr != new_sql_updates.end() )
        {
            if(info.rev > 0 && (info.rev > last_sql_rev[info.db_idx] ||
                (info.rev == last_sql_rev[info.db_idx] && info.nr > last_sql_nr[info.db_idx])))
            {
                last_sql_rev[info.db_idx] = info.rev;
                last_sql_nr[info.db_idx] = info.nr;
                sscanf(buffer, "%[^.]", last_sql_update[info.db_idx]);
            }
            new_sql_updates.erase(itr);
        }
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

bool copy_file(const char *src_file, const char *dst_file)
{
    FILE * fin = fopen( src_file, "rb" );
    if(!fin) return false;
    FILE * fout = fopen( dst_file, "wb" );
    if(!fout) { fclose(fin); return false; }

    for(char c = getc(fin); !feof(fin); putc(c, fout), c = getc(fin));

    fclose(fin);
    fclose(fout);
    return true;
}

bool convert_sql_updates()
{
    if(new_sql_updates.empty()) return true;

    printf("+ converting sql updates\n");

    // rename the sql update files and add the required update statement
    for(std::set<std::string>::iterator itr = new_sql_updates.begin(); itr != new_sql_updates.end(); ++itr)
    {
        sql_update_info info;
        if(!get_sql_update_info(itr->c_str(), info)) return false;
        if(info.db_idx == NUM_DATABASES) return false;

        // generating the new name should work for updates with or without a rev
        char src_file[MAX_PATH], new_name[MAX_PATH], dst_file[MAX_PATH];
        snprintf(src_file, MAX_PATH, "%s%s/%s", path_prefix, sql_update_dir, itr->c_str());
        snprintf(new_name, MAX_PATH, "%d_%0*d_%s%s%s", rev, 2, info.nr, info.db, info.has_table ? "_" : "", info.table);
        snprintf(dst_file, MAX_PATH, "%s%s/%s.sql", path_prefix, sql_update_dir, new_name);

        FILE * fin = fopen( src_file, "r" );
        if(!fin) return false;

        std::ostringstream out_buff;

        // add the update requirements
        out_buff << "ALTER TABLE " << db_version_table[info.db_idx]
                 << " CHANGE COLUMN required_" << last_sql_update[info.db_idx]
                 << " required_" << new_name << " bit;\n\n";

        // skip the first one or two lines from the input
        // if it already contains update requirements
        if(fgets(buffer, MAX_BUF, fin))
        {
            char dummy[MAX_BUF];
            if(sscanf(buffer, "ALTER TABLE %s CHANGE COLUMN required_%s required_%s bit", dummy, dummy, dummy) == 3)
            {
                if(fgets(buffer, MAX_BUF, fin) && buffer[0] != '\n')
                    out_buff << buffer;
            }
            else
                out_buff << buffer;
        }

        // copy the rest of the file
        while(fgets(buffer, MAX_BUF, fin))
            out_buff << buffer;

        fclose(fin);

        FILE * fout = fopen( dst_file, "w" );
        if(!fout) { fclose(fin); return false; }

        fprintf(fout, "%s",out_buff.str().c_str());

        fclose(fout);

        // rename the file in git
        snprintf(cmd, MAX_CMD, "git add %s", dst_file);
        system_switch_index(cmd);

        // delete src file if it different by name from dst file
        if(strncmp(src_file,dst_file,MAX_PATH))
        {
            snprintf(cmd, MAX_CMD, "git rm --quiet %s", src_file);
            system_switch_index(cmd);
        }

        // update the last sql update for the current database
        strncpy(last_sql_update[info.db_idx], new_name, MAX_PATH);
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
    sql_update_info info;

    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        buffer[strlen(buffer) - 1] = '\0';
        if(buffer[strlen(buffer) - 1] != '/' &&
            strncmp(buffer, "Makefile.am", MAX_BUF) != 0)
        {
            if(new_sql_updates.find(buffer) != new_sql_updates.end())
            {
                if(!get_sql_update_info(buffer, info)) return false;
                snprintf(newname, MAX_PATH, "%d_%0*d_%s%s%s.sql", rev, 2, info.nr, info.db, info.has_table ? "_" : "", info.table);
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
        "# Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>\n"
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

    for(std::set<std::string>::iterator itr = file_list.begin(), next; itr != file_list.end(); ++itr)
    {
        next = itr; ++next;
        fprintf(fout, "\t%s%s\n", itr->c_str(), next == file_list.end() ? "" : " \\");
    }

    fprintf(fout,
        "\n## Additional files to include when running 'make dist'\n"
        "#  SQL update files, to upgrade database schema from older revisions\n"
        "EXTRA_DIST = \\\n"
    );

    for(std::set<std::string>::iterator itr = file_list.begin(), next; itr != file_list.end(); ++itr)
    {
        next = itr; ++next;
        fprintf(fout, "\t%s%s\n", itr->c_str(), next == file_list.end() ? "" : " \\");
    }

    fclose(fout);

    snprintf(cmd, MAX_CMD, "git add %s%s/Makefile.am", path_prefix, sql_update_dir);
    system_switch_index(cmd);

    return true;
}

bool change_sql_database()
{
    if(new_sql_updates.empty()) return true;
    printf("+ changing database sql files\n");

    // rename the database files, copy their contents back
    // and change the required update line
    for(int i = 0; i < NUM_DATABASES; i++)
    {
        if(last_sql_update[i][0] == '\0') continue;

        char old_file[MAX_PATH], tmp_file[MAX_PATH], dummy[MAX_BUF];

        snprintf(old_file, MAX_PATH, "%s%s", path_prefix, db_sql_file[i]);
        snprintf(tmp_file, MAX_PATH, "%s%stmp", path_prefix, db_sql_file[i]);

        rename(old_file, tmp_file);

        FILE *fin = fopen( tmp_file, "r" );
        if(!fin) return false;
        FILE *fout = fopen( old_file, "w" );
        if(!fout) return false;

        snprintf(dummy, MAX_CMD, "CREATE TABLE `%s` (\n", db_version_table[i]);
        while(fgets(buffer, MAX_BUF, fin))
        {
            fputs(buffer, fout);
            if(strncmp(buffer, dummy, MAX_BUF) == 0)
                break;
        }

        while(1)
        {
            if(!fgets(buffer, MAX_BUF, fin)) return false;
            if(sscanf(buffer, "  `required_%s`", dummy) == 1) break;
            fputs(buffer, fout);
        }

        fprintf(fout, "  `required_%s` bit(1) default NULL\n", last_sql_update[i]);
        while(fgets(buffer, MAX_BUF, fin))
            fputs(buffer, fout);

        fclose(fin);
        fclose(fout);
        remove(tmp_file);

        snprintf(cmd, MAX_CMD, "git add %s", old_file);
        system_switch_index(cmd);
    }
    return true;
}

bool change_sql_history()
{
    snprintf(cmd, MAX_CMD, "git log HEAD --pretty=\"format:%%H\"");
    if( (cmd_pipe = popen( cmd, "r" )) == NULL )
        return false;

    std::list<std::string> hashes;
    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        buffer[strlen(buffer) - 1] = '\0';
        if(strncmp(origin_hash, buffer, MAX_HASH) == 0)
            break;

        hashes.push_back(buffer);
    }
    pclose(cmd_pipe);
    if(hashes.empty()) return false;    // must have at least one commit
    if(hashes.size() < 2) return true;  // only one commit, ok but nothing to do

    snprintf(cmd, MAX_CMD, "git reset --hard %s", origin_hash);
    system(cmd);

    for(std::list<std::string>::reverse_iterator next = hashes.rbegin(), itr = next++; next != hashes.rend(); ++itr, ++next)
    {
        // stage the changes from the orignal commit
        snprintf(cmd, MAX_CMD, "git cherry-pick -n %s", itr->c_str());
        system(cmd);

        // remove changed and deleted files
        snprintf(cmd, MAX_CMD, "git checkout HEAD %s%s", path_prefix, sql_update_dir);
        system(cmd);

        // remove the newly added files
        snprintf(cmd, MAX_CMD, "git diff --cached --diff-filter=A --name-only %s%s", path_prefix, sql_update_dir);
        if( (cmd_pipe = popen( cmd, "r" )) == NULL )
            return false;

        while(fgets(buffer, MAX_BUF, cmd_pipe))
        {
            buffer[strlen(buffer) - 1] = '\0';
            snprintf(cmd, MAX_CMD, "git rm -f --quiet %s%s", path_prefix, buffer);
            system(cmd);
        }

        pclose(cmd_pipe);

        // make a commit with the same author and message as the original one

        snprintf(cmd, MAX_CMD, "git commit -C %s", itr->c_str());
        system(cmd);
    }

    snprintf(cmd, MAX_CMD, "git cherry-pick %s", hashes.begin()->c_str());
    system(cmd);

    return true;
}

bool prepare_new_index()
{
    if(!use_new_index) return true;

    // only use a new index if there are staged changes that should be preserved
    if( (cmd_pipe = popen( "git diff --cached", "r" )) == NULL ) {
        use_new_index = false;
        return false;
    }

    if(!fgets(buffer, MAX_BUF, cmd_pipe))
    {
        use_new_index = false;
        pclose(cmd_pipe);
        return true;
    }

    pclose(cmd_pipe);

    printf("+ preparing new index\n");

    // copy the existing index file to a new one
    char src_file[MAX_PATH], dst_file[MAX_PATH];

    char *old_index = getenv("GIT_INDEX_FILE");
    if(old_index) strncpy(src_file, old_index, MAX_PATH);
    else snprintf(src_file, MAX_PATH, "%s.git/index", path_prefix);
    snprintf(dst_file, MAX_PATH, "%s%s", path_prefix, new_index_file);

    if(!copy_file(src_file, dst_file)) return false;

    // doesn't seem to work with path_prefix
    snprintf(new_index_cmd, MAX_CMD, "GIT_INDEX_FILE=%s/%s", base_path, new_index_file);
    if(putenv(new_index_cmd) != 0) return false;

    // clear the new index
    system("git reset -q --mixed HEAD");

    // revert to old index
    snprintf(old_index_cmd, MAX_CMD, "GIT_INDEX_FILE=");
    if(putenv(old_index_cmd) != 0) return false;
    return true;
}

bool cleanup_new_index()
{
    if(!use_new_index) return true;
    printf("+ cleaning up the new index\n");
    char idx_file[MAX_PATH];
    snprintf(idx_file, MAX_PATH, "%s%s", path_prefix, new_index_file);
    remove(idx_file);
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
            printf("   -h, --help            show the usage\n");
            printf("   -r, --replace         replace the rev number if it was already applied\n");
            printf("                         to the last commit\n");
            printf("   -l, --local           search for the highest rev number on HEAD\n");
            printf("   -f, --fetch           fetch from origin before searching for the new rev\n");
            printf("   -s, --sql             search for new sql updates and do all of the changes\n");
            printf("                         for the new rev\n");
            printf("       --branch=BRANCH   specify which remote branch to use\n");
            return 0;
        }
    }

    if (local && do_sql)
    {
        printf("Options -l/--local and -s/--sql can't be used in same time currently.\n");
        printf("FAILED\n");
        return 1;
    }

    DO( find_path()                     );
    if(!local)
    {
        DO( find_origin()               );
        if(do_fetch)
            DO( fetch_origin()          );
        DO( check_fwd()                 );
    }
    DO( find_rev()                      );
    DO( find_head_msg()                 );
    if(do_sql)
        DO( find_sql_updates()          );
    DO( prepare_new_index()             );
    DO( write_rev_nr()                  );
    if(do_sql)
    {
        DO( convert_sql_updates()       );
        if (generate_makefile)
            DO( generate_sql_makefile() );
        DO( change_sql_database()       );
        DO( write_rev_sql()             );
    }
    DO( amend_commit()                  );
    DO( cleanup_new_index()             );
    //if(do_sql)
    //    DO( change_sql_history()        );

    return 0;
}
