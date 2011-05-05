#include <defrag.h>
#include <stdlib.h>

int main(char const * args)
{
    int arg = atoi(args);
    int nf;

    switch(arg) {
    case 1:
        nf = nfrags("/mnt/disk/file6");
        if (nf == 3) return EXIT_SUCCESS;
        else         return EXIT_FAILURE;
    }
}
