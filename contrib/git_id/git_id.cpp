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

// config

#define NUM_REMOTES 2

char remotes[NUM_REMOTES][MAX_REMOTE] = {
    "git@github.com:mangos/mangos.git",
    "git://github.com/mangos/mangos.git"        // used for fetch if present
};

char rev_file[] = "src/shared/revision_nr.h";
char sql_update_dir[] = "sql/updates";

bool allow_replace = false;
bool local = false;
bool do_fetch = false;
bool do_sql = false;

// aux

char origins[NUM_REMOTES][MAX_REMOTE];
int rev;

char head_message[MAX_MSG];
char path_prefix[MAX_PATH] = "";
char buffer[MAX_BUF];
char cmd[MAX_CMD];
char origin_hash[MAX_HASH];

std::set<std::string> sql_updates;

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
    snprintf(cmd, MAX_CMD, "git fetch %s master", origins[1][0] ? origins[1] : origins[0]);
    int ret = system(cmd);
    return true;
}

bool check_fwd()
{
    printf("+ checking fast forward\n");
    snprintf(cmd, MAX_CMD, "git log -n 1 --pretty=\"format:%%H\" %s/master", origins[1][0] ? origins[1] : origins[0]);
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
        else sprintf(cmd, "git log %s/master --pretty=\"format:%%s\"", origins[i]);
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
    printf("+ finding sql updates on HEAD\n");
    // add all updates from HEAD
    snprintf(cmd, MAX_CMD, "git show HEAD:%s", sql_update_dir);
    if( (cmd_pipe = popen( cmd, "r" )) == NULL )
        return false;

    // skip first two lines
    if(!fgets(buffer, MAX_BUF, cmd_pipe)) { pclose(cmd_pipe); return false; }
    if(!fgets(buffer, MAX_BUF, cmd_pipe)) { pclose(cmd_pipe); return false; }

    int nr;
    char db_table[MAX_BUF];

    while(fgets(buffer, MAX_BUF, cmd_pipe))
    {
        buffer[strnlen(buffer, MAX_BUF) - 1] = '\0';
        if(sscanf(buffer, "%d_%s.sql", &nr, db_table) == 2)
            sql_updates.insert(buffer);
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
        if(sscanf(buffer, "%d_%s.sql", &nr, db_table) == 2)
            sql_updates.erase(buffer);
    }

    pclose(cmd_pipe);
    return true;
}

bool rename_sql_updates()
{
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
        if(strncmp(argv[i], "-l", 2) == 0 || strncmp(argv[i], "--local", 7) == 0)
            local = true;
        if(strncmp(argv[i], "-f", 2) == 0 || strncmp(argv[i], "--fetch", 7) == 0)
            do_fetch = true;
        if(strncmp(argv[i], "-s", 2) == 0 || strncmp(argv[i], "--sql", 5) == 0)
            do_sql = true;
        if(strncmp(argv[i], "-h", 2) == 0 || strncmp(argv[i], "--help", 6) == 0)
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
        DO( rename_sql_updates() );
    }
    DO( amend_commit()  );

    return 0;
}
