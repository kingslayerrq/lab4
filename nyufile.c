#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void printFSInfo(){
    
}

int main(int argc, char **argv)
{
    int opt;
    if ((opt = getopt(argc, argv, "ilr:R:")) != -1)
    {
        // fprintf(stderr, "argc = %d\n", argc);
        if (argc == 3)
        {
            if (opt == 'i')
            {
                fprintf(stderr, "i\n");
            }
            else if (opt == 'l')
            {
                fprintf(stderr, "l\n");
            }
            else
            {
                fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                return 0;
            }
        }
        else if (argc == 4)
        {
            if (opt == 'r' && optarg)
            {
                fprintf(stderr, "r then nothing\n");
            }
            else
            {
                fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                return 0;
            }
        }
        else if (argc == 6)
        {
            if (opt == 'r' && optarg)
            {
                if ((opt = getopt(argc, argv, "s:") != -1) && strcmp(optarg, "sha1") == 0)
                {
                    fprintf(stderr, "r then s\n");
                }
                else
                {
                    fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                    return 0;
                }
            }
            else if (opt == 'R' && optarg)
            {
                if ((opt = getopt(argc, argv, "s:") != -1) && strcmp(optarg, "sha1") == 0)
                {
                    fprintf(stderr, "R then s\n");
                }
                else
                {
                    fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                    return 0;
                }
            }
            else
            {
                fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                return 0;
            }
        }
        else
        {
            fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
            return 0;
        }
    }
    else
    {
        fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
        return 0;
    }
}