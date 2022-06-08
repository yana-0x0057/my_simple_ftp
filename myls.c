#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <string.h>
#define LSINFOMAX 2048

void printperm(struct stat st, char *buf)
{
    mode_t status = st.st_mode;
    char d = '-', or = '-', ow = '-', ox = '-', gr = '-', gw = '-', gx = '-', ur = '-', uw = '-', ux = '-';

    if (S_ISDIR(status))
    {
        d = 'd';
    }
    if (S_ISLNK(status))
    {
        d = '|';
    }
    if (S_ISREG(status))
    {
        d = '-';
    }
    if (S_ISSOCK(status))
    {
        d = 's';
    }
    if (S_ISBLK(status))
    {
        d = 'b';
    }
    if (S_ISCHR(status))
    {
        d = 'c';
    }
    if (S_ISFIFO(status))
    {
        d = 'p';
    }

    if (status & S_IRUSR)
    {
        or = 'r';
    }
    if (status & S_IWUSR)
    {
        ow = 'w';
    }
    if (status & S_IXUSR)
    {
        ox = 'x';
    }

    if (status & S_IRGRP)
    {
        gr = 'r';
    }
    if (status & S_IWGRP)
    {
        gw = 'w';
    }
    if (status & S_IXGRP)
    {
        gx = 'x';
    }

    if (status & S_IROTH)
    {
        ur = 'r';
    }
    if (status & S_IWOTH)
    {
        uw = 'w';
    }
    if (status & S_IXOTH)
    {
        ux = 'x';
    }
    snprintf(buf, 22, "%c%c%c%c%c%c%c%c%c%c", d, or, ow, ox, gr, gw, gx, ur, uw, ux);
}

int myls(int n, char **pname, char *output)
{
    struct stat st;
    DIR *dir = NULL;
    struct dirent *dp = NULL;
    struct passwd *user = NULL;
    struct group *group = NULL;
    struct tm *time = NULL;
    char filename[256], filekindbuf[22], buf[128];
    safecpy(filename, "", 256);
    safecpy(filekindbuf, "", 256);
    safecpy(buf, "", 128);
    int dir_flag = 0;

    if (n > 2)
    {
        fprintf(stderr, "usage: ldir [path]\n");
        return -1;
    }

    if (n == 1)
    {
        if (stat(".", &st) < 0)
        {
            perror("stat");
            return -1;
        }
        if ((dir = opendir(".")) == NULL)
        {
            perror("open");
            return -1;
        }
        dir_flag = 1;
    }
    if (n == 2)
    {
        if (stat(pname[1], &st) < 0)
        {
            perror(pname[1]);
            return -1;
        }
        if (S_ISDIR(st.st_mode))
        {
            if ((dir = opendir(pname[1])) == NULL)
            {
                perror("open");
                return -1;
            }
            dir_flag = 1;
        }
        else
        {
            safecpy(filename, pname[1], 256);
        }
    }

    if (dir_flag == 1)
    {
        for (dp = readdir(dir); dp != NULL; dp = readdir(dir))
        {
            stat(dp->d_name, &st);
            time = localtime(&st.st_atime);
            printperm(st, filekindbuf);
            snprintf(buf, 128, "%3d %10lld %d %d %02d:%02d %s\n", st.st_nlink, st.st_size, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, dp->d_name);
            strncat(output, filekindbuf, 22);
            strncat(output, buf, 1024);
        }
        closedir(dir);
    }
    else
    {
        time = localtime(&st.st_atime);
        printperm(st, filekindbuf);
        snprintf(buf, 128, "%3d %10lld %d %d %02d:%02d %s\n", st.st_nlink, st.st_size, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, dp->d_name);
        strncat(output, filekindbuf, 22);
        strncat(output, buf, 1024);
    }
    return 0;
}
