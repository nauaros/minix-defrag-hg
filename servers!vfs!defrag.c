/* This file contains the implementation of system calls related to file
 * fragmentaton.
 *
 * The entry points into this file are
 *  do_nfrags: performs the nfrags system call
 *  do_defrag: performs the defrag system call
 */

#include "fs.h"
#include "param.h"
#include "vnode.h"

/*===========================================================================*
 *				do_nfrags				     *
 *===========================================================================*/

PUBLIC int do_nfrags()
{
  int r;
  unsigned int nfrags;
  struct vnode *vp;

  /* Put the file name in user_fullpath. */
  r = fetch_name(m_in.name, m_in.name_length, M3);
  if (r != OK) return(err_code);	/* name was bad */

  /* Get vnode from file name. */
  vp = eat_path(PATH_NOFLAGS, fp);
  if (vp == NULL) return(err_code);	/* invalid path */

  /* Check for file type. */
  if (!(vp->v_mode & I_REGULAR)) {
  	if	(vp->v_mode & I_DIRECTORY)	return(EISDIR);
  	else if	(vp->v_mode & I_BLOCK_SPEACIAL)	return(EISSPECIAL);
  }

  /* Check to see if file is used somewhere else. */
  if (vp->v_ref_count != 1) return(EISINUSE);

  /* Request the mfs server to perform the call. */
  return(req_frags(vp->v_fs_e, vp->v_inode_nr, FALSE));
}

/*===========================================================================*
 *				do_defrag				     *
 *===========================================================================*/

PUBLIC int do_defrag()
{
  int r;

  /* r = req_frags(vp->v_fs_e, vp->v_inode_nr, TRUE); */
}
