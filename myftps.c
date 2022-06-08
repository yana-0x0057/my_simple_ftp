#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "getargs.c"
#include "myls.c"
#include "ftph.c"
#include "file.c"
#define BUFMAX 512
#define PORT 50021
struct sockaddr_in srv, cli;
struct in_addr ip_cli;
in_port_t port_srv, port_cli;
socklen_t sktlen_srv, sktlen_cli;
char buffer[DATASIZE];
int s, s2, count, datalen;
int quit();
int pwd();
int cwd(char *);
int list(char *);
int retr(char *);
int stor(char *);

void sigchld_handler()
{
    int st;
    wait(&st);
}

int main(int argc, char *argv[])
{
    int n = 0, pid, st;
    sktlen_cli = sizeof cli;
    signal(SIGCHLD, sigchld_handler);
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    if (argc > 3)
    {
        fprintf(stderr, "usage: myftps [dir]\n");
        return -1;
    }
    if (argc == 2)
    {
        if (chdir(argv[1]) < 0)
        {
            fprintf(stderr, "cannnot change dir\n");
            return -1;
        }
    }
    port_srv = PORT;
    memset(&srv, 0, sizeof srv);
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port_srv);
    srv.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&srv, sizeof srv) < 0)
    {
        perror("bind");
        exit(1);
    }
    if (listen(s, 5) < 0)
    {
        perror("listen");
        exit(1);
    }
    while (1)
    {
        safecpy(buffer, "", BUFMAX);
        if ((s2 = accept(s, (struct sockaddr *)&cli, &sktlen_cli)) < 0)
        {
            perror("accept");
        }

        if ((pid = fork()) < 0)
        {
            perror("fork");
            return -1;
        }
        else if (pid == 0)
        {
            daemon(1, 0);
            while (1)
            {
                uint8_t type = 0, code = 0;
                uint16_t length = 0;
                struct myftph *rcv = NULL;
                rcv = mallocmsg(0);
                char *msg = NULL;
                if (recv(s2, rcv, sizeof(struct myftph), 0) < 0)
                {
                    perror("recv");
                }
                type = rcv->type;
                code = rcv->code;
                length = rcv->length;
                if ((length = rcv->length) != 0)
                {
                    msg = (char *)malloc(length);
                    if (recv(s2, msg, length, 0) < 0)
                    {
                        perror("recv");
                    }
                }
                switch (type)
                {
                case 0x01:
                    fprintf(stderr, "quit\n");
                    quit();
                    break;
                case 0x02:
                    fprintf(stderr, "pwd\n");
                    pwd();
                    break;
                case 0x03:
                    fprintf(stderr, "cwd\n");
                    cwd(msg);
                    break;
                case 0x04:
                    fprintf(stderr, "list\n");
                    if (rcv->length != 0)
                    {
                        list(msg);
                    }
                    else
                    {
                        list(".");
                    }
                    break;
                case 0x05:
                    fprintf(stderr, "retr\n");
                    retr(msg);
                    break;
                case 0x06:
                    fprintf(stderr, "stor\n");
                    stor(msg);
                    break;
                default:
                    fprintf(stderr, "err: no command.\n");
                    break;
                }
            }
            exit(0);
        }
        else
        {
            close(s2);
        }
    }
}

int quit()
{
    close(s2);
    exit(0);
    return 0;
}

int pwd()
{
    size_t length;
    char wd[BUFMAX];
    safecpy(wd, "", BUFMAX);
    if (getcwd(wd, BUFMAX) == NULL)
    {
        perror("current dir");
        return -1;
    }
    length = strlen(wd);
    struct myftph *m = NULL;
    m = mallocmsg(DATASIZE);
    m->type = 0x10;
    m->code = 0x00;
    m->length = length;
    safecpy(m->data, wd, length + 1);
    if (send(s2, m, sizeof(struct myftph) + m->length, 0) < 0)
    {
        perror("send");
        fprintf(stderr, "cannot send msg\n");
        return -1;
    }
    return 0;
}

int cwd(char *dir)
{
    struct myftph *m = NULL;
    m = mallocmsg(0);
    if (chdir(dir) < 0)
    {
        perror("chdir");
        m->type = 0x12;
        if (errno == ENOENT)
        {
            m->code = 0x00;
        }
        else if (errno == EACCES)
        {
            m->code = 0x01;
        }
        else
        {
            m->type = 0x13;
            m->code = 0x05;
        }
        m->length = 0;
        if (send(s2, m, sizeof(struct myftph), 0) < 0)
        {
            perror("send");
            return -1;
        }
        return -1;
    }
    else
    {
        m->type = 0x10;
        m->code = 0x00;
        m->length = 0;
        if (send(s2, m, sizeof(struct myftph), 0) < 0)
        {
            perror("send");
            return -1;
        }
    }
    return 0;
}

int list(char *path)
{
    char buf[LSINFOMAX];
    safecpy(buf, "", DATASIZE);
    char *dammy[] = {"", path};
    int sendfin_flag = 0;
    char *p;
    int pos = 0;

    if (myls(2, dammy, buf) < 0)
    {
        fprintf(stderr, "myls error.\n");
        struct myftph *m = NULL;
        m = mallocmsg(0);
        m->type = 0x11;
        m->code = 0x00;
        m->length = 0;
        if (send(s2, m, sizeof(struct myftph), 0) < 0)
        {
            perror("send");
            return -1;
        }
        return -1;
    }
    struct myftph *m = NULL;
    m = mallocmsg(0);
    m->type = 0x10;
    m->code = 0x01;
    m->length = 0;
    if (send(s2, m, sizeof(struct myftph), 0) < 0)
    {
        perror("send");
        return -1;
    }
    pos = 0;
    size_t len_all = strlen(buf);
    while (sendfin_flag == 0)
    {
        struct myftph *r = NULL;
        size_t length = len_all - pos;
        char data[DATASIZE];
        if (length <= DATASIZE)
        {
            sendfin_flag = 1;
            strncpy(data, buf + pos, length);
        }
        else
        {
            strncpy(data, buf + pos, DATASIZE);
            length = DATASIZE;
            pos = pos + DATASIZE;
        }
        r = mallocmsg(length);
        r->type = 0x20;
        if (sendfin_flag == 1)
        {
            r->code = 0x00;
        }
        else
        {
            r->code = 0x01;
        }
        r->length = length;
        strncpy(r->data, data, length);
        if (send(s2, r, sizeof(struct myftph) + length, 0) < 0)
        {
            perror("send");
            return -1;
        }
        free(r);
    }

    return 0;
}

int retr(char *path)
{
    struct myftph *m = NULL;
    int sendfin_flag = 0, fd = 0;
    struct stat st;
    size_t filesize = 0;
    int pos = 0;
    if (stat(path, &st) < 0)
    {
        perror("stat");
        return -1;
    }
    filesize = st.st_size;
    if ((fd = bufcpycheck(path)) < 0)
    {
        m = mallocmsg(0);
        m->type = 0x11;
        m->code = 0x00;
        m->length = 0;
        if (send(s2, m, sizeof(struct myftph), 0) < 0)
        {
            perror("send");
            return -1;
        }
        return -1;
    }
    m = mallocmsg(0);
    m->type = 0x10;
    m->code = 0x01;
    m->length = 0;
    if (send(s2, m, sizeof(struct myftph), 0) < 0)
    {
        perror("send");
        close(fd);
        return -1;
    }
    while (sendfin_flag == 0)
    {
        struct myftph *datamsg = NULL;
        size_t length = filesize - pos;

        if (length <= DATASIZE)
        {
            datamsg = mallocmsg(length);
            int n = 0;
            sendfin_flag = 1;
            if ((n = read(fd, datamsg->data, length)) < 0)
            {
                perror("read");
                close(fd);
                free(datamsg);
                return -1;
            }
        }
        else
        {
            int n = 0;
            datamsg = mallocmsg(DATASIZE);
            if ((n = read(fd, datamsg->data, DATASIZE)) < 0)
            {
                perror("read");
                free(datamsg);
                close(fd);
                return -1;
            }
            length = DATASIZE;
            pos = pos + DATASIZE;
        }
        datamsg->type = 0x20;
        datamsg->length = length;
        if (sendfin_flag == 1)
        {
            datamsg->code = 0x00;
        }
        else
        {
            datamsg->code = 0x01;
        }
        if (send(s2, datamsg, sizeof(struct myftph) + length, 0) < 0)
        {
            perror("send");
            close(fd);
            return -1;
        }
        free(datamsg);
    }
    close(fd);
    return 0;
}

int stor(char *path)
{
    struct myftph *m = NULL;
    char buf[DATASIZE];
    int recvfin_flag = 0;
    safecpy(buf, "", DATASIZE);
    size_t length = 0;
    if (filecheck(path) < 0)
    {
        m = mallocmsg(0);
        m->type = 0x12;
        m->code = 0x00;
        m->length = 0;
        if (send(s2, m, sizeof(struct myftph), 0) < 0)
        {
            perror("send");
            return -1;
        }
        fprintf(stderr, "err: no path\n");
        return -1;
    }

    m = mallocmsg(0);
    m->type = 0x10;
    m->code = 0x02;
    m->length = 0;
    if (send(s2, m, sizeof(struct myftph), 0) < 0)
    {
        perror("send");
        return -1;
    }

    int fd = 0;
    if ((fd = myopen(path)) < 0)
    {
        fprintf(stderr, "error: open\n");
        return -1;
    }
    while (recvfin_flag == 0)
    {
        struct myftph *datamsg = NULL;
        size_t datalen = 0;
        datamsg = mallocmsg(0);
        if (recv(s2, datamsg, sizeof(struct myftph), 0) < 0)
        {
            perror("recv");
            return -1;
        }
        if (datamsg->type != 0x20)
        {
            fprintf(stderr, "error: get wrong type msg.\n");
            return -1;
        }
        if (datamsg->code == 0)
        {
            recvfin_flag = 1;
        }
        datalen = datamsg->length;
        if (recv(s2, buf, datalen, 0) < 0)
        {
            perror("recv");
            close(fd);
            return -1;
        }
        if (write(fd, buf, datalen) < 0)
        {
            perror("write");
            close(fd);
            return -1;
        }
        free(datamsg);
    }
    close(fd);
    return 0;
}