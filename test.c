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
        if (nf == -1) {
            printf("nfrags KO\n");
            switch(errno) {
            case EISDIR:       printf("EISDIR\n");       break;
            case EFTYPE:       printf("EFTYPE\n");       break;
            case EBUSY:        printf("EBUSY\n");        break;

            /* returned by fetch_name */
            case ENAMETOOLONG: printf("ENAMETOOLONG\n"); break;
            case EINVAL:       printf("EINVAL\n");       break;

            /* returned by eat_path */
            case ENOENT:       printf("ENOENT\n");       break;
            case ELOOP:        printf("ELOOP\n");        break;
            case E2BIG:        printf("E2BIG\n");        break;
            }
        }
        else if (nf == 3) {
            printf("nfrags OK\n");
        } else  {
            printf("unexpected return value\n");
        }
    }
    return EXIT_SUCCESS;
}
