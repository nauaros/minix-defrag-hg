/* This file contains the implementation of system calls related to file
 * fragmentaton.
 *
 * The entry points into this file are
 *  do_nfrags: performs the nfrags system call
 *  do_defrag: performs the defrag system call
 */

#include "fs.h"
#include <minix/vfsif.h>
#include "param.h"
#include "vnode.h"

/*===========================================================================*
 *				do_nfrags				     *
 *===========================================================================*/

PUBLIC int do_nfrags()
{
  return common_frags(FALSE);
}

/*===========================================================================*
 *				do_defrag				     *
 *===========================================================================*/

PUBLIC int do_defrag()
{
  return common_frags(TRUE);
}

/*===========================================================================*
 *				common_frags				     *
 *===========================================================================*/

int common_frags(defrag)
int defrag;			/* TRUE if defragmentation requested */
{
  int r;			/* return value	*/
  struct vnode *vp;		/* file vnode	*/
  mode_t file_mode;		/* file mode 	*/

  /* Put the file name in user_fullpath. */
  r = fetch_name(m_in.name, m_in.name_length, M3);
  if (r != OK) return(err_code);	/* name was bad */

  /* Get vnode from file name. */
  vp = eat_path(PATH_NOFLAGS, fp);
  if (vp == NULL) return(err_code);	/* invalid path */

  /* Check for file type. */
  if (!(vp->v_mode & I_REGULAR)) {
	file_mode = vp->v_mode;
	put_vnode(vp);
  	if	(file_mode & I_DIRECTORY)	return(EISDIR);
  	else if	(file_mode & I_BLOCK_SPECIAL)	return(EFTYPE);
  }

  /* Check to see if file is used somewhere else. */
  if (vp->v_ref_count != 1) return(EBUSY);

  /* Request the fs server to perform the call. */
  r = req_frags(vp->v_fs_e, vp->v_inode_nr, defrag);

  put_vnode(vp);
  return(r);
}
