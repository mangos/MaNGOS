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

#define NUM_REMOTES 2

char remotes[NUM_REMOTES][256] = {
    "git@github.com:mangos/mangos.git",
    "git://github.com/mangos/mangos.git"
};

char origins[NUM_REMOTES][256];
int rev;
char head_message[16384];
char write_file[2048];

char buffer[256];
FILE *cmd_pipe;

bool allow_replace = false;
bool local = false;

bool find_path()
{
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
    if( (cmd_pipe = popen( "git remote -v", "rt" )) == NULL )
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
    // find the highest rev number on either of the remotes
    for(int i = 0; i < NUM_REMOTES; i++)
    {
        if(!local && !origins[i][0]) continue;

        char cmd[512];
        if(local) sprintf(cmd, "git log HEAD --pretty=\"format:%%s\"");
        else sprintf(cmd, "git log %s/master --pretty=\"format:%%s\"", origins[i]);
        if( (cmd_pipe = popen( cmd, "rt" )) == NULL )
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
    if( (cmd_pipe = popen( "git log -n 1 --pretty=\"format:%s%n%n%b\"", "rt" )) == NULL )
        return false;

    int poz = 0;
    while(poz < 16384-1 && EOF != (head_message[poz++] = fgetc(cmd_pipe)));
    head_message[poz-1] = '\0';

    pclose(cmd_pipe);

    if(get_rev(head_message))
    {
        if(!allow_replace) return false;
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
    char cmd[512];
    sprintf(cmd, "git commit --amend -F- %s", write_file);
    if( (cmd_pipe = popen( cmd, "wt" )) == NULL )
        return false;

    fprintf(cmd_pipe, "[%d] %s", rev, head_message);
    pclose(cmd_pipe);
    
    return true;
}

int main(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        if(argv[i] == NULL) continue;
        if(strncmp(argv[i], "-r", 2) == 0)
            allow_replace = true;
        if(strncmp(argv[i], "-l", 2) == 0)
            local = true;
        if(strncmp(argv[i], "-h", 2) == 0)
        {

        }
    }

    if(!find_path())              { printf("ERROR: can't find path\n");         return 1; }
    if(!local && !find_origin())  { printf("ERROR: can't find origin\n");       return 1; }
    if(!find_rev())               { printf("ERROR: can't find rev\n");          return 1; }
    if(!find_head_msg())          { printf("ERROR: can't find head message\n"); return 1; }
    if(!write_rev())              { printf("ERROR: can't write revision_nr.h\n");  return 1; }
    if(!amend_commit())           { printf("ERROR: can't ammend commit\n");     return 1; }
    
    printf("Generated rev %d\n", rev);

    return 0;
}