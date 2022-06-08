#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#define COMMANDMAX 13
#define WORDMAX 50
#define LINEMAX 35

void safecpy(char *fname, char *cont, int memsize)
{
    memset(fname, 0, memsize);
    strncpy(fname, cont, memsize - 1);
}

int getargs(int *argc, char *argv[])
{

    int n = 0, m = 0, ac = *argc;
    char line[LINEMAX], c, mode = 0;
    int rd = 0;
    int fd;

    memset(line, 0, LINEMAX);
    while ((c = getchar()) != EOF && c != '\0' && n < LINEMAX - 1)
    {
        line[n] = c;

        if (c == '\n')
        {
            break;
        }
        n++;
    }
    if (c == EOF)
    {
        return 1;
    }
    n = 0;
    while (line[m] != '\0' && line[m] != '\n' && ac < COMMANDMAX - 1)
    {
        argv[ac] = (char *)malloc(WORDMAX * sizeof(char));
        memset(argv[ac], 0, WORDMAX);
        mode = 0;
        while (line[m] == ' ' || line[m] == '\t')
        {
            m++;
        }
        for (n = 0; line[m] != ' ' && line[m] != '\0' && line[m] != '\n' && n < WORDMAX && line[m] != '\t'; n++, m++)
        {
            if (line[m] == '|' || line[m] == '>' || line[m] == '<')
            {
                mode = line[m];
                if (n != 0)
                {
                    argv[ac][n] = '\0';
                    ac++;
                    argv[ac] = (char *)malloc(2 * sizeof(char));
                }
                argv[ac][0] = mode;
                argv[ac][1] = '\0';
                m++;
                ac++;
                break;
            }
            argv[ac][n] = line[m];
        }
        if (n == WORDMAX && line[m] != '\0')
        {
            fprintf(stderr, "Error: too many characters to input.\n");
            ac++;
            fflush(stdin);
            break;
        }
        if (mode == 0)
        {
            argv[ac][n] = '\0';
            ac++;
        }
    }

    if (line[0] == '\n')
    {
        argv[ac] = "\n\0";
    }

    if ((line[m] == '\n') && (line[m - 1] == ' ' || line[m = 1] == '\t') && ac > 1)
        ac--;
    if (ac >= COMMANDMAX - 1)
    {
        fflush(stdin);
        fprintf(stderr, "Error: too many commands to input. :%d\n", ac);
    }
    *argc = ac;
    return 0;
}
