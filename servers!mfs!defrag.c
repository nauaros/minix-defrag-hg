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
  zone_t current, previous;
  block_t blockno, blockno2;
  struct buf *bp, *bp2;
  int zi, bi, izi, zi2, bi2, izi2, nblocks, nzones, nfrags, scale, zone_size;

  /* Find the inode corresponding to the passed vnode. */
  ip = find_inode(fs_dev, (ino_t) fs_m_in.REQ_INODE_NR);
  if (ip == NULL) return(EINVAL);

  /* byte order ? */
  /* always at least one block ? */
  /* describe variables */

  /* Number of blocks and zones in file. */
  scale = ip->i_sp->s_log_zone_size;	/* for block-zone conversion */
  nblocks = ip->i_size / ip->i_sp->s_block_size;
  if (ip->i_size % ip->i_sp->s_block_size != 0) nblocks += 1;
  nzones = nblocks >> scale;
  if (nblocks - nzones << scale != 0) nzones += 1;

  nfrags = 1;
  previous = ip->i_zone[0];

  /* Count fragments among direct zones. */
  for (zi = 1 ; zi < ip->i_ndzones && zi < nzones ; zi++) {
  	current = ip->zone[zi];
   	if (current != previous + 1) nfrags++;
  	previous = current;
  }

  zone_size = 1 << scale;	/* # blocks/zone */
  blockno = current << scale;	/* current block number */

  /* Initialization of the values for the doubly indirect loop. */
  /* WONT WORK MUST MAKE AUXILIARY FUNCTIONS */
  blockno2 = ip->zone[zi+1] << scale;
  bi2 = 0;
  izi2 = 0;

  /* Count fragments among indirect zones. */
indirect:
  /* For all blocks in current (indirect) zone. */
  for (bi = 0 ; bi < zone_size && zi < nzones ; bi++) {
  	/* Pointer to current block. */
  	bp = get_block(ip->i_dev, blockno++, NORMAL);
  	/* For all zone numbers in current block. */
  	for (izi = 0 ; izi < ip->i_nindirs && zi < nzones ; izi++, zi++) {
  		current = rd_indir(bp, izi);
  		if (current != previous + 1) nfrags++;
  		previous = current;
  	}
  	put_block(bp, INDIRECT_BLOCK);
  }

  /* Count fragments among doubly indirect zones. */
  for (; bi2 < zone_size && zi < nzones ; bi2++) {
  	bp2 = get_block(ip->i_dev, blockno2++, NORMAL);
  	for (; izi2 < ip->i_nindirs && zi < nzones ; izi2++, zi++)
  		goto indirect;
  	izi2 = 0;
  	put_block(bp2, INDIRECT_BLOCK);
  }

  if (nfrags != 1 && fs_m_in.REQ_DEFRAG_FLAG) {
  	/* DO DEFRAG */
  }

  return(nfrags); /* to modify */
}
