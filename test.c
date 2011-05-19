#include <defrag.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argv, char const * args[])
{
    int arg;
    int nf;

    arg = atoi(args[1]);
    switch(arg) {

    /* test if nfrags reports the correct number of fragments */
    case 1:
        nf = nfrags("/mnt/disk/file6");
        printf("nfrags returned: %d\n", nf);
        if (nf == -1) printf("errno: %d\n", errno);
        break;
    case 2:
        nf = defrag("/mnt/disk/file6");
        printf("defrag returned %d\n", nf);
        if (nf == -1) printf("errno: %d\n", errno);
        break;
    }
    return EXIT_SUCCESS;
}
