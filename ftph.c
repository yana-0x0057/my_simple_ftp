#include <stdio.h>
#include <stdlib.h>
#define DATASIZE 1024
struct myftph
{
    uint8_t type;
    uint8_t code;
    uint16_t length;
    char data[0];
};

struct myftph *mallocmsg(int datasize)
{
    struct myftph *ans = NULL;
    if (datasize > DATASIZE)
    {
        fprintf(stderr, "error: too mach datasize.\n");
        return NULL;
    }
    ans = (struct myftph *)malloc(sizeof(struct myftph) + datasize);
    if (ans == NULL)
    {
        fprintf(stderr, "cannot allocate memory\n");
        return NULL;
    }
    memset(ans, 0, sizeof(struct myftph) + datasize);
    return ans;
}
