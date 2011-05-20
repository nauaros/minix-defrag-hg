#include <defrag.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * args[1] is "defrag" or "nfrags"
 * args[2] is a file path
 */
int main(int argv, char const * args[])
{
    int nf;
    if (!strcmp(args[1], "nfrags")) {
        nf = nfrags(args[2]);
        printf("nfrags returned: %d\n", nf);
        if (nf == -1) printf("errno: %d\n", errno);
    }
    else if (!strcmp(args[1], "defrag")) {
        nf = defrag(args[2]);
        printf("defrag returned %d\n", nf);
        if (nf == -1) printf("errno: %d\n", errno);
    }
    else printf("bad argument: %s\n", args[1]);
    return EXIT_SUCCESS;
}
