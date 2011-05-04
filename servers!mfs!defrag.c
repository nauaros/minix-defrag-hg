/* This files contains the mfs support for the system calls related to file
 * framentation.
 */

#include "fs.h"

/*===========================================================================*
 *				fs_frags				     *
 *===========================================================================*/

PUBLIC int fs_frags(void)
{
  struct inode *ip;
  int scale, blk_in_zone, zone_size, nfrags;
  offset_t off;
  block_t expected;

  /* Find the inode corresponding to the passed vnode. */
  ip = find_inode(fs_dev, (ino_t) fs_m_in.REQ_INODE_NR);
  if (ip == NULL) return(EINVAL);

  scale = ip->i_sp->s_log_zone_size;	/* for block-zone conversion */
  blk_in_zone = 1 << scale;		/* # blocks/zone */
  zone_size = ip->i_sp->block_size << scale;	/* zone size in byte */
  nfrags = 1;					/* # fragments */

  /* sizeof block_t ? */
  /* Expected block for next offset if next zone is contiguous. */
  if (ip->sp->s_version == V1)
  	expected = (block_t) conv2(sp->s_native, (int)  ip->izone[0] << scale);
  else
  	expected = (block_t) conv4(sp->s_native, (long) ip->izone[0] << scale);

  /* Jump from zone to zone and increment nfrags when there is a
     discontinuity. */
  for (off = zone_size ; off < ip->i_size ; off += zone_size) {
  	expected += blk_in_zone;
  	if (read_map(ip, off) != expected) nfrags++;
  }

  if (nfrags != 1 && fs_m_in.REQ_DEFRAG_FLAG) {
  	/* DO DEFRAG */
  }

  return(nfrags); /* to modify */
}
