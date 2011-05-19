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
  int r;
  struct vnode *vp;
  mode_t mode;

  /* Put the file name in user_fullpath. */
  r = fetch_name(m_in.name, m_in.name_length, M3);
  if (r != OK) return(err_code);	/* name was bad */

  /* Get vnode from file name. */
  vp = eat_path(PATH_NOFLAGS, fp);
  if (vp == NULL) return(err_code);	/* invalid path */

  /* Check for file type. */
  if (!(vp->v_mode & I_REGULAR)) {
	mode = vp->v_mode;
	put_vnode(vp);
  	if	(mode & I_DIRECTORY)	return(EISDIR);
  	else if	(mode & I_BLOCK_SPECIAL)return(EFTYPE);
  }

  /* Check to see if file is used somewhere else. */
  if (vp->v_ref_count != 1) return(EBUSY);

  /* Request the fs server to perform the call. */
  int r = req_frags(vp->v_fs_e, vp->v_inode_nr, FALSE);

  put_vnode(vp);
  return(r);
}

/*===========================================================================*
 *				do_defrag				     *
 *===========================================================================*/

PUBLIC int do_defrag()
{
  int r;
  return 43;
  /* r = req_frags(vp->v_fs_e, vp->v_inode_nr, TRUE); */
}
