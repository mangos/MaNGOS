#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <assert.h>
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

// config

#define NUM_REMOTES 2

char remotes[NUM_REMOTES][256] = {
    "git@github.com:mangos/mangos.git",
    "git://github.com/mangos/mangos.git"        // used for fetch if present
};

bool allow_replace = false;
bool local = false;
bool do_fetch = false;

// aux

char origins[NUM_REMOTES][256];
int rev;
char head_message[16384];
char write_file[2048];

char buffer[256];
FILE *cmd_pipe;

bool find_path()
{
    printf("+ finding path\n");
    char cur_path[2048], *ptr;
    getcwd(cur_path, 2048);
    int len = strnlen(cur_path, 2048);
    
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

    char prefix[2048] = "", path[2048];
    for(int i = 0; i < count; i++)
    {
        snprintf(path, 2048, "%s.git", prefix);
        if(0 == chdir(path))
        {
            chdir(cur_path);
            snprintf(write_file, 2048, "%ssrc/shared/revision_nr.h", prefix);
            return true;
        }
        strncat(prefix, "../", 2048);
    }

    return false;
}

bool find_origin()
{
    printf("+ finding origin\n");
    if( (cmd_pipe = popen( "git remote -v", "r" )) == NULL )
        return false;

    bool ret = false;
    while(fgets(buffer, 256, cmd_pipe))
    {
        char name[256], remote[256];
        sscanf(buffer, "%s %s", name, remote);
        for(int i = 0; i < NUM_REMOTES; i++)
        {
            if(strcmp(remote, remotes[i]) == 0)
            {
                strncpy(origins[i], name, 256);
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
    char cmd[256];
    // use the public clone url if present because the private may require a password
    sprintf(cmd, "git fetch %s master", origins[1][0] ? origins[1] : origins[0]);
    int ret = system(cmd);
    return true;
}

bool check_fwd()
{
    printf("+ checking fast forward\n");
    char cmd[256];
    sprintf(cmd, "git log -n 1 --pretty=\"format:%%H\" %s/master", origins[1][0] ? origins[1] : origins[0]);
    if( (cmd_pipe = popen( cmd, "r" )) == NULL )
        return false;

    char hash[256];
    if(!fgets(buffer, 256, cmd_pipe)) return false;
    strncpy(hash, buffer, 256);
    pclose(cmd_pipe);

    if( (cmd_pipe = popen( "git log --pretty=\"format:%H\"", "r" )) == NULL )
        return false;

    bool found = false;
    while(fgets(buffer, 256, cmd_pipe))
    {
        buffer[strnlen(buffer, 256) - 1] = '\0';
        if(strncmp(hash, buffer, 256) == 0)
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

        char cmd[512];
        if(local) sprintf(cmd, "git log HEAD --pretty=\"format:%%s\"");
        else sprintf(cmd, "git log %s/master --pretty=\"format:%%s\"", origins[i]);
        if( (cmd_pipe = popen( cmd, "r" )) == NULL )
            continue;

        int nr;
        while(fgets(buffer, 256, cmd_pipe))
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

    if(FILE* OutputFile = fopen(write_file,"wb"))
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
    char cmd[512];
    sprintf(cmd, "git commit --amend -F- %s", write_file);
    if( (cmd_pipe = popen( cmd, "w" )) == NULL )
        return false;

    fprintf(cmd_pipe, "[%d] %s", rev, head_message);
    pclose(cmd_pipe);
    
    return true;
}

#define DO(cmd) if(!cmd) { printf("FAILED\n"); return 1; }

int main(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        if(argv[i] == NULL) continue;
        if(strncmp(argv[i], "-r", 2) == 0 || strncmp(argv[i], "--replace", 2) == 0)
            allow_replace = true;
        if(strncmp(argv[i], "-l", 2) == 0 || strncmp(argv[i], "--local", 2) == 0)
            local = true;
        if(strncmp(argv[i], "-f", 2) == 0 || strncmp(argv[i], "--fetch", 2) == 0)
            do_fetch = true;
        if(strncmp(argv[i], "-h", 2) == 0 || strncmp(argv[i], "--help", 2) == 0)
        {
            printf("Usage: git_id [OPTION]\n");
            printf("Generates a new rev number and updates revision_nr.h and the commit message.\n");
            printf("Should be used just before push.\n");
            printf("   -h, --help     show the usage\n");
            printf("   -r, --replace  replace the rev number if it was already applied to the last\n");
            printf("                  commit\n");
            printf("   -l, --local    search for the highest rev number on HEAD\n");
            printf("   -f, --fetch    fetch from origin before searching for the new rev\n");
            return 0;
        }
    }

    DO( find_path()     );
    if(!local)
    {
        DO( find_origin()   );
        if(do_fetch)
        {
            DO( fetch_origin()  );
            DO( check_fwd()     );
        }
    }
    DO( find_rev()      );
    DO( find_head_msg() );
    DO( write_rev()     );
    DO( amend_commit()  );

    return 0;
}
