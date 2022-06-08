#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define FNSIZE 80
#define MAXLEN 1000

int bufcpy(char *fn, char *contents)
{
    int fd;
    char filename[FNSIZE];
    safecpy(filename, fn, FNSIZE);
    if ((fd = open(filename, O_RDONLY)) < 0)
    {
        perror("open");
        return -1;
    }
    else
    {
        memset(contents, 0, MAXLEN);
        if (read(fd, contents, MAXLEN - 1) < 0)
        {
            perror("copy");
            return -1;
        }
    }
    close(fd);
    return 0;
}

void mycat(char *fn)
{
    int fd;
    char contents[MAXLEN];
    fd = bufcpy(fn, contents);
    printf("%s", contents);
}

int myopen(char *de)
{
    int fd, m;
    char dest[FNSIZE];
    safecpy(dest, de, FNSIZE);
    if ((fd = open(dest, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0)
    {
        if (errno != EEXIST)
        {
            fprintf(stderr, "%s", dest);
            perror("open dest file");
            return -1;
        }
        if ((fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
        {
            perror("open");
            return -1;
        }
    }
    return fd;
}

int mycp(char *so, char *de, int n)
{
    int fd, m;
    char buff[DATASIZE];
    safecpy(buff, so, DATASIZE);
    if ((fd = myopen(de)) < 0)
    {
        fprintf(stderr, "error: open\n");
        return -1;
    }
    for (m = 0; m < MAXLEN; m += n)
    {
        write(fd, buff, n);
        lseek(fd, n, SEEK_CUR);
    }
    close(fd);
    return 0;
}

int filecheck(char *path)
{
    int fd;
    char dest[FNSIZE];
    safecpy(dest, path, FNSIZE);

    if ((fd = open(dest, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0)
    {
        if (errno != EEXIST)
        {
            fprintf(stderr, "%s", dest);
            perror("open dest file");
            return -1;
        }
        else if ((fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
        {
            perror("open");
            return -1;
        }
    }
    close(fd);
    return 0;
}

int bufcpycheck(char *fn)
{
    int fd;
    char filename[FNSIZE];
    safecpy(filename, fn, FNSIZE);
    if ((fd = open(filename, O_RDONLY)) < 0)
    {
        perror("open");
        return -1;
    }
    return fd;
}