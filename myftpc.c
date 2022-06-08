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
#include "getargs.c"
#include "myls.c"
#include "ftph.c"
#include "file.c"
#define BUFMAX 1024
#define PORT 50021
struct sockaddr_in srv, cli;
in_port_t port_srv, port_cli;
socklen_t sktlen_srv;
struct in_addr ip_srv;
int s, count, datalen;

int quit(int n, char *[]);
int lpwd(int n, char *[]);
int lcd(int n, char *[]);
int ldir(int n, char *[]);
int put(int n, char *[]);
int help(int n, char *[]);
int pwd(int n, char *[]);
int cd(int n, char *[]);
int dir(int n, char *[]);
int get(int n, char *[]);

struct command_table
{
    char *cmd;
    int (*func)(int, char *[]);
};
struct command_table cmd_tbl_cli[] = {
    {"quit", quit},
    {"lpwd", lpwd},
    {"lcd", lcd},
    {"ldir", ldir},
    {"put", put},
    {"help", help},
    {NULL, NULL}

};
struct command_table cmd_tbl_srv[] = {
    {"pwd", pwd},
    {"cd", cd},
    {"dir", dir},
    {"get", get},
    {NULL, NULL}

};

int main(int argc, char *argv[])
{
    int myargc, cmd_flag;
    char *myargv[BUFMAX];
    struct command_table *mycmd;

    sktlen_srv = sizeof srv;
    if (argc != 2)
    {
        fprintf(stderr, "usage: ./myftpc server-IP-address\n");
        exit(1);
    }

    if (inet_aton(argv[1], &ip_srv) < 0)
    {
        perror("ipaddr");
    }
    port_srv = PORT;

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    memset(&srv, 0, sizeof srv);
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port_srv);
    srv.sin_addr.s_addr = ip_srv.s_addr;

    if (connect(s, (struct sockaddr *)&srv, sizeof srv) < 0)
    {
        perror("connect");
        exit(1);
    }

    while (1)
    {
        myargc = 0;
        printf("myFTP%% ");
        getargs(&myargc, myargv);
        for (mycmd = cmd_tbl_cli; mycmd->cmd != NULL; mycmd++)
        {
            if (strcmp(myargv[0], mycmd->cmd) == 0)
            {
                (*mycmd->func)(myargc, myargv);
                break;
            }
        }
        if (mycmd->cmd == NULL)
        {
            for (mycmd = cmd_tbl_srv; mycmd->cmd != NULL; mycmd++)
            {
                if (strcmp(myargv[0], mycmd->cmd) == 0)
                {
                    (*mycmd->func)(myargc, myargv);
                    cmd_flag = 1;
                    break;
                }
            }
        }
        if (mycmd->cmd == NULL)
        {
            fprintf(stderr, "Error: command not found.\n");
        }
    }
    return 0;
}

int quit(int n, char **m)
{
    struct myftph *msg = NULL;
    msg = mallocmsg(0);
    msg->type = 0x01;
    msg->code = 0;
    msg->length = 0;
    if (send(s, msg, sizeof(struct myftph), 0) < 0)
    {
        perror("send");
        return -1;
    }
    exit(0);
    return 0;
}
int lpwd(int n, char *m[])
{
    char wd[BUFMAX];
    safecpy(wd, "", BUFMAX);
    getcwd(wd, BUFMAX);
    printf("%s\n", wd);
    return 0;
};
int lcd(int n, char *m[])
{
    if (chdir(m[1]) < 0)
    {
        perror("local change directory:");
        return -1;
    };
    return 0;
}

int ldir(int n, char **m)
{
    char buf[LSINFOMAX];
    safecpy(buf, "", BUFMAX);
    myls(n, m, buf);
    printf("%s", buf);
    return 0;
}

int pwd(int n, char **m)
{
    struct myftph *msg = NULL, *rmsg = NULL;
    char buf[DATASIZE];
    safecpy(buf, "", DATASIZE);
    msg = mallocmsg(0);
    msg->type = 0x02;
    msg->code = 0;
    msg->length = 0;
    if (send(s, msg, sizeof(struct myftph), 0) < 0)
    {
        perror("send");
        return -1;
    }
    rmsg = mallocmsg(0);
    if (recv(s, rmsg, sizeof(struct myftph), 0) < 0)
    {
        perror("recv");
        return -1;
    }
    if (recv(s, buf, rmsg->length, 0) < 0)
    {
        perror("recv");
        return -1;
    }
    printf("%s\n", buf);
    return 0;
}

int cd(int n, char *m[])
{
    if (n != 2)
    {
        fprintf(stderr, "usage: cd directory\n");
        return -1;
    }
    else
    {
        struct myftph *msg = NULL, *rmsg = NULL;
        size_t length = strlen(m[1]);
        msg = mallocmsg(length);
        rmsg = mallocmsg(0);
        msg->type = 0x03;
        msg->length = length;
        safecpy(msg->data, m[1], length + 1);
        if (send(s, msg, sizeof(struct myftph) + length, 0) < 0)
        {
            perror("send");
            return -1;
        }
        if (recv(s, rmsg, sizeof(struct myftph), 0) < 0)
        {
            perror("recv");
            return -1;
        }
        if (!(rmsg->type == 0x10 && rmsg->code == 0x00))
        {
            if (rmsg->code == 0x00)
            {
                fprintf(stderr, "error: no dir %s in the server\n", m[1]);
                return -1;
            }
            else if (rmsg->code == 0x01)
            {
                fprintf(stderr, "error: no permission to access dir %s in the server\n", m[1]);
                return -1;
            }
            else
            {
                fprintf(stderr, "unknown error\n");
                return -1;
            }
            return -1;
        }
    }
    return 0;
}

int dir(int n, char **m)
{
    struct myftph *msg = NULL, *rmsg = NULL;
    int end_flag = 0;
    size_t length = 0;
    rmsg = mallocmsg(0);
    if (n > 3)
    {
        fprintf(stderr, "usage: dir [path]\n");
        return -1;
    }
    if (n == 2)
    {
        length = strlen(m[1]);
        msg = mallocmsg(length);
        msg->length = length;
        safecpy(msg->data, m[1], length + 1);
    }
    else
    {
        msg = mallocmsg(0);
        msg->length = 0;
    }
    msg->type = 0x04;
    msg->code = 0;
    if (send(s, msg, sizeof(struct myftph) + length, 0) < 0)
    {
        perror("send");
        return -1;
    }
    if (recv(s, rmsg, sizeof(struct myftph), 0) < 0)
    {
        perror("recv");
        return -1;
    }
    if (rmsg->type == 0x10 && rmsg->code == 0x01)
    {
        free(rmsg);
        while (end_flag == 0)
        {
            size_t msglen = 0;
            char buf[DATASIZE];
            safecpy(buf, "", DATASIZE);
            struct myftph *datamsg = NULL;
            datamsg = mallocmsg(0);
            if (recv(s, datamsg, sizeof(struct myftph), 0) < 0)
            {
                perror("recv");
                return -1;
            }
            if (datamsg->type != 0x20)
            {
                fprintf(stderr, "error: something is wrong. datamsg->type = 0x%hhx\n", datamsg->type);
                return -1;
            }
            if (datamsg->code == 0)
            {
                end_flag = 1;
            }
            msglen = datamsg->length;
            if (recv(s, buf, msglen, 0) < 0)
            {
                perror("recv");
                return -1;
            }
            printf("%s", buf);
            free(datamsg);
        }
    }
    else
    {
        fprintf(stderr, "error: cannot get information.\n");
        return -1;
    }
    return 0;
}

int get(int n, char **path)
{
    struct myftph *msg = NULL, *rmsg = NULL;
    char buf[DATASIZE];
    int end_flag = 0;
    safecpy(buf, "", DATASIZE);
    if (n < 2 || n > 4)
    {
        fprintf(stderr, "usage: get serverpath [clientpath]\n");
        return -1;
    }
    size_t length = strlen(path[1]);
    msg = mallocmsg(length);
    msg->type = 0x05;
    msg->length = length;
    safecpy(msg->data, path[1], length + 1);
    if (send(s, msg, sizeof(struct myftph) + length, 0) < 0)
    {
        perror("send");
        return -1;
    }
    rmsg = mallocmsg(0);
    if (recv(s, rmsg, sizeof(struct myftph), 0) < 0)
    {
        perror("recv");
        return -1;
    }
    if (!(rmsg->type == 0x10 && rmsg->code == 0x01))
    {
        fprintf(stderr, "error: get error code.\n");
        return -1;
    }

    int fd = 0;
    if ((fd = myopen(path[n - 1])) < 0)
    {
        fprintf(stderr, "error: open\n");
        return -1;
    }

    while (end_flag == 0)
    {
        size_t datalen;
        struct myftph *dmsg = NULL;
        dmsg = mallocmsg(0);
        if (recv(s, dmsg, sizeof(struct myftph), 0) < 0)
        {
            perror("recv");
            close(fd);
            return -1;
        }
        if (dmsg->type != 0x20)
        {
            fprintf(stderr, "error: get wrong msg\n");
            close(fd);
            return -1;
        }
        if (dmsg->code == 0)
        {
            end_flag = 1;
        }
        datalen = dmsg->length;
        if (recv(s, buf, datalen, 0) < 0)
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
        free(dmsg);
    }
    close(fd);
    return 0;
}

int put(int n, char **path)
{
    struct myftph *msg = NULL, *rmsg = NULL;
    struct stat st;
    int sendfin_flag = 0, pos = 0, fd = 0;
    size_t length = 0, filesize = 0;
    if (n < 2 || n > 4)
    {
        fprintf(stderr, "usage: get serverpath [clientpath]\n");
        return -1;
    }

    msg = mallocmsg(DATASIZE);
    msg->type = 0x06;
    msg->code = 0;
    if (n == 2)
    {
        length = strlen(path[1]);
        msg->length = length;
        safecpy(msg->data, path[1], length + 1);
    }
    else
    {
        length = strlen(path[2]);
        msg->length = length;
        safecpy(msg->data, path[2], length + 1);
    }
    if (send(s, msg, sizeof(struct myftph) + length, 0) < 0)
    {
        perror("send");
        return -1;
    }
    rmsg = mallocmsg(0);
    if (recv(s, rmsg, sizeof(struct myftph), 0) < 0)
    {
        perror("recv");
        return -1;
    }
    if (!(rmsg->type == 0x10 && rmsg->code == 0x02))
    {
        fprintf(stderr, "error: get error msg.\n");
        return -1;
    }
    if (stat(path[1], &st) < 0)
    {
        perror("stat");
        return -1;
    }
    if ((fd = bufcpycheck(path[1])) < 0)
    {
        fprintf(stderr, "file open\n");
        return -1;
    }
    filesize = st.st_size;
    pos = 0;
    while (sendfin_flag == 0)
    {
        struct myftph *dmsg = NULL;
        size_t msglen = filesize - pos;

        if (msglen <= DATASIZE)
        {
            dmsg = mallocmsg(msglen);
            int n = 0;
            sendfin_flag = 1;
            if ((n = read(fd, dmsg->data, msglen)) < 0)
            {
                perror("read");
                close(fd);
                return -1;
            }
        }
        else
        {
            dmsg = mallocmsg(DATASIZE);
            int n = 0;
            if ((n = read(fd, dmsg->data, DATASIZE)) < 0)
            {
                perror("read");
                close(fd);
                return -1;
            }
            msglen = DATASIZE;
            pos = pos + DATASIZE;
        }

        dmsg->type = 0x20;
        if (sendfin_flag == 1)
        {
            dmsg->code = 0x00;
        }
        else
        {
            dmsg->code = 0x01;
        }
        dmsg->length = msglen;
        if (send(s, dmsg, sizeof(struct myftph) + msglen, 0) < 0)
        {
            perror("send");
            close(fd);
            return -1;
        }
        free(dmsg);
    }
    close(fd);
    return 0;
}

int help(int n, char **m)
{
    printf("This is a help messagae.\n");
    printf("You can use these commands.\n");
    printf("quit : end this program\n");
    printf("pwd  : print the current directory in the server.\n");
    printf("cd   : change the current directory in the server.\n");
    printf("dir  : print the information of the file in the server.\n");
    printf("lpwd : print the current directory in the client.\n");
    printf("lcd  : change the current directory in the client.\n");
    printf("ldir : print the information of the file in the client.\n");
    printf("get  : get a file to the server.\n");
    printf("put  : send a file to the server.\n");
    printf("help : print help messages.\n");
    return 0;
}