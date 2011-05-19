/* This files contains the mfs support for the system calls related to file
 * framentation.
 */

#include "fs.h"
#include "inode.h"
#include "super.h"

/*===========================================================================*
 *				fs_frags				     *
 *===========================================================================*/

PUBLIC int fs_frags(void)
{
  struct inode *ip;
  struct super_block *sp;
  int scale, blk_in_zone, zone_size, nfrags;
  off_t off;
  block_t expected;
  zone_t dest;
  int n;

  /* Find the inode corresponding to the passed vnode. */
  ip = find_inode(fs_dev, (ino_t) fs_m_in.REQ_INODE_NR);
  if (ip == NULL) return(EINVAL);

  sp = ip->i_sp;
  scale = sp->s_log_zone_size;	/* for block-zone conversion */
  blk_in_zone = 1 << scale;		/* # blocks/zone */
  zone_size = sp->s_block_size << scale;	/* zone size in byte */
  nfrags = 1;					/* # fragments */

  /* Expected block for next offset if next zone is contiguous. */
  if (sp->s_version == V1)
  	expected = (block_t) conv2(sp->s_native, (int)  ip->i_zone[0] << scale);
  else
  	expected = (block_t) conv4(sp->s_native, (long) ip->i_zone[0] << scale);

  /* Jump from zone to zone and increment nfrags when there is a
     discontinuity. */
  for (off = zone_size ; off < ip->i_size ; off += zone_size) {
  	expected += blk_in_zone;
  	if (read_map(ip, off) != expected) nfrags++;
  }
  fs_m_out.RES_NFRAGS = nfrags;

  /* Don't defrag. */
  if (nfrags == 1 || !fs_m_in.REQ_DEFRAG_FLAG) return(OK);

  blk = ip->i_size + sp->s_block_size - 1) / sp->s_block_size;
  n = blk >> scale + (blk % blk_in_zones > 0);
  dest = alloc_n_zones(ip->i_dev, n);

  while

  return(OK);
}

