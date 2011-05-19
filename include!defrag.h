/* defrag.h - Definition of system calls related to file fragmentation. */

#ifndef _DEFRAG_H
#define _DEFRAG_H

#ifndef _ANSI_H
#include <ansi.h>
#endif

_PROTOTYPE( int nfrags, (const char *file)				);
_PROTOTYPE( int defrag, (const char *file)				);

#endif /* _DEFRAG_H */
