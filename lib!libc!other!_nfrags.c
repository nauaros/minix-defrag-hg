#include <lib.h>
#define nfrags	_nfrags
#include <defrag.h>

PUBLIC int nfrags(file)
  const char * file;
{
  message m;
  _loadname(file, &m);
  return(_syscall(VFS_PROC_NR, NFRAGS, &m));
}
