#include <lib.h>
#define defrag _defrag
#include <defrag.h>

PUBLIC int defrag(file)
  const char * file;
{
  message m;
  _loadname(file, &m);
  return(_syscall(VFS_PROC_NR, NFRAGS, &m));
}
