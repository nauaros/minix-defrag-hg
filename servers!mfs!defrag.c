/* This files contains the mfs support for the system calls related to file
 * framentation.
 */

#include "fs.h"
#include "buf.h"
#include "inode.h"
#include "super.h"

/*===========================================================================*
 *				fs_frags				     *
 *===========================================================================*/

PUBLIC int fs_frags(void)
{
/* Pass back the number of fragments of a file in fs_m_out.RES_NFRAGS, and
 * defragments the file if fs_m_in.REQ_DEFRAG_FLAG == TRUE. The file is
 * specified by its inode number given in fs_m_in.REQ_INODE_NR. */

  struct inode *ip;		/* pointer to the file's inode		*/
  struct super_block *sp;	/* file's filesystem super-block	*/
  unsigned scale;	        /* for bloc-zone conversion		*/
  unsigned blk_in_zone;		/* # blocs / zone			*/
  unsigned zone_size;		/* zone size in bytes			*/
  int nfrags;			/* number of fragments in the file	*/
  off_t off;			/* offset in the file (in bytes)	*/
  block_t blk;			/* a block number (temp variable)	*/
  block_t src_blk, dst_blk;	/* block number to copy from/to		*/
  struct buf *buf_src;		/* block buffer to copy from		*/
  struct buf *buf_dst;		/* block buffer to copy to		*/
  int n;			/* number of zones in the file */

  /* Number of next zone in the file after defragmentation. This will be used to
   * replace and old zone number (from before the defragmentation). */
  zone_t zone;

  /* First block of the zone contiguous to the zone in which the last offset
   * was (the offset was in the first block of that zone). */
  block_t expected;

  /* Find the inode corresponding to the passed number. */
  ip = find_inode(fs_dev, (ino_t) fs_m_in.REQ_INODE_NR);
  if (ip == NULL) return(EINVAL);

  /* Constants initialization. */
  sp = ip->i_sp;
  scale = sp->s_log_zone_size;
  blk_in_zone = 1 << scale;
  zone_size = sp->s_block_size << scale;

  nfrags = 0;			/* empty file have 0 fragments		*/
  expected = 0;			/* file data can't be in block 0	*/

  /* Jump from zone to zone and increment nfrags when there is a
   * discontinuity. */
  for (off = 0 ; off < ip->i_size ; off += zone_size) {
  	if ((blk = read_map(ip, off)) != expected) {
		expected = blk;
		nfrags++;
	}
	expected += blk_in_zone;
  }

  fs_m_out.RES_NFRAGS = nfrags;
  if (nfrags <= 1 || !fs_m_in.REQ_DEFRAG_FLAG) return(OK); /* don't defrag */
  printf("defrag mode entered\n");

  /* # of blocks in the file. */
  dst_blk = (ip->i_size + sp->s_block_size - 1) / sp->s_block_size;
  /* # of zones in the file */
  n = dst_blk >> scale + (dst_blk % blk_in_zone > 0);

  zone = alloc_n_zones(ip->i_dev, n);	/* number of first zone		*/
  dst_blk = zone << scale;		/* number of first block	*/

  /* Copy all blocks to their new location. */
  for (off = 0; off < ip->i_size ;) {
	printf("iter copy : %d\n", off);
	src_blk = read_map(ip, off);
	buf_src = get_block(ip->i_dev, src_blk, NORMAL);
	buf_dst = get_block(ip->i_dev, dst_blk, NORMAL);
	memcpy(buf_dst->b_data, buf_src->b_data, sp->s_block_size);

	/* Increment offset, reuse n to store block type. */
        if ((off += sp->s_block_size) <= ip->i_size) n = FULL_DATA_BLOCK;
	else n = PARTIAL_DATA_BLOCK;

	put_block(buf_src, n);
	put_block(buf_dst, n);
	++dst_blk;
  }

#if 0
  /* Copy all blocks to their new location. */
  blk = zone << scale;
  for (off = 0; off < ip->i_size ; off += sp->s_block_size) {
	src_blk = read_map(ip, off);
	buf_src = get_block(ip->i_dev, src_blk, NORMAL);
	buf_src->b_blocknr = blk++;

	/* Increment offset, reuse n to store block type. */
        if ((off += sp->s_block_size) <= ip->i_size) n = FULL_DATA_BLOCK;
	else n = PARTIAL_DATA_BLOCK;

	put_block(buf_src, n);
  }
#endif

  /* Update all inode numbers. */
  for (off = 0 ; off < ip->i_size ; off += zone_size) {
	printf("iter inode : %d\n", off);
	write_map(ip, off, zone++, WMAP_FREE); }

  return(OK);
}

