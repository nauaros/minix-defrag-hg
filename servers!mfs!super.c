/* This file manages the super block table and the related data structures,
 * namely, the bit maps that keep track of which zones and which inodes are
 * allocated and which are free.  When a new inode or zone is needed, the
 * appropriate bit map is searched for a free entry.
 *
 * The entry points into this file are
 *   alloc_bit:       somebody wants to allocate a zone or inode; find one
 *   alloc_n_bits     find a contiguous block of n zones or inodes to allocate
 *   free_bit:        indicate that a zone or inode is available for allocation
 *   get_super:       search the 'superblock' table for a device
 *   mounted:         tells if file inode is on mounted (or ROOT) file system
 *   read_super:      read a superblock
 */

#include "fs.h"
#include <string.h>
#include <minix/com.h>
#include <minix/u64.h>
#include "buf.h"
#include "inode.h"
#include "super.h"
#include "const.h"

/*===========================================================================*
 *				alloc_n_bits				     *
 *===========================================================================*/

PUBLIC bit_t alloc_n_bits(sp, map, origin, n)
struct super_block *sp;		/* the filesystem to allocate from	*/
int map;			/* IMAP (inode map) or ZMAP (zone map)	*/
bit_t origin;			/* number of bit to start searching at	*/
int n;				/* the number of bits to allocate != 0	*/
{
/* Allocate a string of n contiguous bits from a bit map and return the bit
   number of the first bit. */

  block_t start_block;		/* first bit's block			*/
  bit_t map_bits;		/* # bits in bit map			*/
  short bit_blocks;		/* # blocks in bit map	       		*/
  struct buf *bp;		/* current block			*/
  block_t block;		/* index of bp inside bit_blocks	*/
  unsigned word;		/* index of starting chunk for bp	*/
  unsigned bcount;		/* # remaining blocks to iterate upon	*/
  bitchunk_t *wlim;		/* addr after last chunk of bp		*/
  bitchunk_t *wptr;		/* addr of current chunk in bp		*/
  bitchunk_t k;			/* current chunk in bp			*/
  bit_t i;			/* current bit in k                     */
  int free_bits;		/* current # of contiguous free bits	*/
  bit_t b;			/* number of first allocated bit	*/

  if (sp->s_rd_only)
	panic("can't allocate bit on read-only filesys");

  if (map == IMAP) {
	start_block = START_BLOCK;
	map_bits = (bit_t) (sp->s_ninodes + 1);
	bit_blocks = sp->s_imap_blocks;
  } else {
	start_block = START_BLOCK + sp->s_imap_blocks;
	map_bits = (bit_t) (sp->s_zones - (sp->s_firstdatazone - 1));
	bit_blocks = sp->s_zmap_blocks;
  }

  /* Figure out where to start the bit search (depends on 'origin'). */
  if (origin >= map_bits) origin = 0;	/* for robustness */

  /* Locate the starting place. */
  block = (block_t) (origin / FS_BITS_PER_BLOCK(sp->s_block_size));
  word = (origin % FS_BITS_PER_BLOCK(sp->s_block_size)) / FS_BITCHUNK_BITS;

  /* Initialization of counters. */
  free_bits = 0;
  bp = get_block(sp->s_dev, start_block + block, NORMAL);
  wlim = &bp->b_bitmap[FS_BITMAP_CHUNKS(sp->s_block_size)];
  wptr = &bp->b_bitmap[word];
  /* Iterate over all blocks plus one, because we start in the middle. */
  bcount = bit_blocks + 1;

  /* Find a string of n free bits or return NO_BIT. */
  while (TRUE) {

	/* Go faster : does this chunk contain a free bit? */
	if (*wptr == (bitchunk_t) ~0) {
		free_bits = 0;
		goto iter;
	}

	/* Get current chunk. */
	k = (bitchunk_t) conv2(sp->s_native, (int) *wptr);

	/* Get contiguous bits in current block. */
	for (i = 0 ; i < FS_BITCHUNK_BITS ; ++i) {
  		if ((k & (1 << i)) == 0) {
			if (++free_bits == n) break;
		}
		else free_bits = 0;
	}

	if (free_bits == n) {
		/* String found : get the number of the first bit. */
		b = ((bit_t) block * FS_BITS_PER_BLOCK(sp->s_block_size))
		    + (wptr - &bp->b_bitmap[0]) * FS_BITCHUNK_BITS
		    + i + 1 - n;

		/* String not ok : there are bits beyond the end of the map. */
		if (b + n > map_bits) free_bits = 0;
		else break;
	}

iter:
	/* Next chunk, and if needed, next block. */
	if (++wptr == wlim) {
		put_block(bp, MAP_BLOCK);

		/* All blocks have been checked, no string could be found. */
		if (--bcount == 0) return(NO_BIT);

		/* Last block, wrap around. */
		if (++block >= (unsigned int) bit_blocks) block = 0;

		bp = get_block(sp->s_dev, start_block + block, NORMAL);
		wlim = &bp->b_bitmap[FS_BITMAP_CHUNKS(sp->s_block_size)];
		wptr = &bp->b_bitmap[0];
	}
  }

  ++i;				/* i = index of last bit + 1 */

  /* While we have not reached the first chunk, allocate bits belows i in last
     chunk and all bits encountered in other chunks. */
  while (b % FS_BITCHUNK_BITS + n - 1 >= FS_BITCHUNK_BITS) {

	printf("non-first chunk: %d\n", b+n);

	/* Allocate bits in current chunk. */
	k |= (bitchunk_t) ~0 >> FS_BITCHUNK_BITS - i;
	*wptr = (bitchunk_t) conv2(sp->s_native, (int) k);

	/* Get previous chunk, and if needed previous block. */
	if (--wptr < &bp->b_bitmap[0]) {
		bp->b_dirt = DIRTY;
		put_block(bp, MAP_BLOCK);
		--block;
		bp = get_block(sp->s_dev, start_block + block, NORMAL);
		wptr = &bp->b_bitmap[FS_BITMAP_CHUNKS(sp->s_block_size) - 1];
	}
	k = (bitchunk_t) conv2(sp->s_native, (int) *wptr);

	n -= i ;
	i = FS_BITCHUNK_BITS;
  }

  printf("first chunk: %d\n", b);

  /* Reuse i as the index of the first bit in the first chunk. */
  i = b % FS_BITCHUNK_BITS;
  /* Allocate bits in the first chunk. */
  k |= (bitchunk_t) ~0 << i & (bitchunk_t) ~0 >> FS_BITCHUNK_BITS - n - i;
  printf("k: %x\n", k);
  *wptr = (bitchunk_t) conv2(sp->s_native, (int) k);
  bp->b_dirt = DIRTY;
  put_block(bp, MAP_BLOCK);

  return(b);
}

/*===========================================================================*
 *				alloc_bit				     *
 *===========================================================================*/
PUBLIC bit_t alloc_bit(sp, map, origin)
struct super_block *sp;		/* the filesystem to allocate from */
int map;			/* IMAP (inode map) or ZMAP (zone map) */
bit_t origin;			/* number of bit to start searching at */
{
/* Allocate a bit from a bit map and return its bit number. */

  block_t start_block;		/* first bit block */
  block_t block;
  bit_t map_bits;		/* how many bits are there in the bit map? */
  short bit_blocks;		/* how many blocks are there in the bit map? */
  unsigned word, bcount;
  struct buf *bp;
  bitchunk_t *wptr, *wlim, k;
  bit_t i, b;

  if (sp->s_rd_only)
	panic("can't allocate bit on read-only filesys");

  if (map == IMAP) {
	start_block = START_BLOCK;
	map_bits = (bit_t) (sp->s_ninodes + 1);
	bit_blocks = sp->s_imap_blocks;
  } else {
	start_block = START_BLOCK + sp->s_imap_blocks;
	map_bits = (bit_t) (sp->s_zones - (sp->s_firstdatazone - 1));
	bit_blocks = sp->s_zmap_blocks;
  }

  /* Figure out where to start the bit search (depends on 'origin'). */
  if (origin >= map_bits) origin = 0;	/* for robustness */

  /* Locate the starting place. */
  block = (block_t) (origin / FS_BITS_PER_BLOCK(sp->s_block_size));
  word = (origin % FS_BITS_PER_BLOCK(sp->s_block_size)) / FS_BITCHUNK_BITS;

  /* Iterate over all blocks plus one, because we start in the middle. */
  bcount = bit_blocks + 1;
  do {
	bp = get_block(sp->s_dev, start_block + block, NORMAL);
	wlim = &bp->b_bitmap[FS_BITMAP_CHUNKS(sp->s_block_size)];

	/* Iterate over the words in block. */
	for (wptr = &bp->b_bitmap[word]; wptr < wlim; wptr++) {

		/* Does this word contain a free bit? */
		if (*wptr == (bitchunk_t) ~0) continue;

		/* Find and allocate the free bit. */
		k = (bitchunk_t) conv2(sp->s_native, (int) *wptr);
		for (i = 0; (k & (1 << i)) != 0; ++i) {}

		/* Bit number from the start of the bit map. */
		b = ((bit_t) block * FS_BITS_PER_BLOCK(sp->s_block_size))
		    + (wptr - &bp->b_bitmap[0]) * FS_BITCHUNK_BITS
		    + i;

		/* Don't allocate bits beyond the end of the map. */
		if (b >= map_bits) break;

		/* Allocate and return bit number. */
		k |= 1 << i;
		*wptr = (bitchunk_t) conv2(sp->s_native, (int) k);
		bp->b_dirt = DIRTY;
		put_block(bp, MAP_BLOCK);
		return(b);
	}
	put_block(bp, MAP_BLOCK);
	if (++block >= (unsigned int) bit_blocks) /* last block, wrap around */
		block = 0;
	word = 0;
  } while (--bcount > 0);
  return(NO_BIT);		/* no bit could be allocated */
}

/*===========================================================================*
 *				free_bit				     *
 *===========================================================================*/
PUBLIC void free_bit(sp, map, bit_returned)
struct super_block *sp;		/* the filesystem to operate on */
int map;			/* IMAP (inode map) or ZMAP (zone map) */
bit_t bit_returned;		/* number of bit to insert into the map */
{
/* Return a zone or inode by turning off its bitmap bit. */

  unsigned block, word, bit;
  struct buf *bp;
  bitchunk_t k, mask;
  block_t start_block;

  if (sp->s_rd_only)
	panic("can't free bit on read-only filesys");

  if (map == IMAP) {
	start_block = START_BLOCK;
  } else {
	start_block = START_BLOCK + sp->s_imap_blocks;
  }
  block = bit_returned / FS_BITS_PER_BLOCK(sp->s_block_size);
  word = (bit_returned % FS_BITS_PER_BLOCK(sp->s_block_size))
  	 / FS_BITCHUNK_BITS;

  bit = bit_returned % FS_BITCHUNK_BITS;
  mask = 1 << bit;

  bp = get_block(sp->s_dev, start_block + block, NORMAL);

  k = (bitchunk_t) conv2(sp->s_native, (int) bp->b_bitmap[word]);
  if (!(k & mask)) {
  	if (map == IMAP) panic("tried to free unused inode");
  	else panic("tried to free unused block: %u", bit_returned);
  }

  k &= ~mask;
  bp->b_bitmap[word] = (bitchunk_t) conv2(sp->s_native, (int) k);
  bp->b_dirt = DIRTY;

  put_block(bp, MAP_BLOCK);
}


/*===========================================================================*
 *				get_super				     *
 *===========================================================================*/
PUBLIC struct super_block *get_super(
  dev_t dev			/* device number whose super_block is sought */
)
{
  if (dev == NO_DEV)
  	panic("request for super_block of NO_DEV");

  if(superblock.s_dev != dev)
  	panic("wrong superblock: %d", (int) dev);

  return(&superblock);
}


/*===========================================================================*
 *				get_block_size				     *
 *===========================================================================*/
PUBLIC unsigned int get_block_size(dev_t dev)
{
  if (dev == NO_DEV)
  	panic("request for block size of NO_DEV");

  return(fs_block_size);

}


/*===========================================================================*
 *				read_super				     *
 *===========================================================================*/
PUBLIC int read_super(sp)
register struct super_block *sp; /* pointer to a superblock */
{
/* Read a superblock. */
  dev_t dev;
  unsigned int magic;
  int version, native, r;
  static char *sbbuf;
  block_t offset;

  STATICINIT(sbbuf, _MIN_BLOCK_SIZE);

  dev = sp->s_dev;		/* save device (will be overwritten by copy) */
  if (dev == NO_DEV)
  	panic("request for super_block of NO_DEV");

  r = block_dev_io(MFS_DEV_READ, dev, SELF_E, sbbuf, cvu64(SUPER_BLOCK_BYTES),
  		   _MIN_BLOCK_SIZE);
  if (r != _MIN_BLOCK_SIZE)
  	return(EINVAL);

  memcpy(sp, sbbuf, sizeof(*sp));
  sp->s_dev = NO_DEV;		/* restore later */
  magic = sp->s_magic;		/* determines file system type */

  /* Get file system version and type. */
  if (magic == SUPER_MAGIC || magic == conv2(BYTE_SWAP, SUPER_MAGIC)) {
	version = V1;
	native  = (magic == SUPER_MAGIC);
  } else if (magic == SUPER_V2 || magic == conv2(BYTE_SWAP, SUPER_V2)) {
	version = V2;
	native  = (magic == SUPER_V2);
  } else if (magic == SUPER_V3) {
	version = V3;
  	native = 1;
  } else {
	return(EINVAL);
  }

  /* If the super block has the wrong byte order, swap the fields; the magic
   * number doesn't need conversion. */
  sp->s_ninodes =           (ino_t) conv4(native, (int) sp->s_ninodes);
  sp->s_nzones =          (zone1_t) conv2(native, (int) sp->s_nzones);
  sp->s_imap_blocks =       (short) conv2(native, (int) sp->s_imap_blocks);
  sp->s_zmap_blocks =       (short) conv2(native, (int) sp->s_zmap_blocks);
  sp->s_firstdatazone_old =(zone1_t)conv2(native,(int)sp->s_firstdatazone_old);
  sp->s_log_zone_size =     (short) conv2(native, (int) sp->s_log_zone_size);
  sp->s_max_size =          (off_t) conv4(native, sp->s_max_size);
  sp->s_zones =             (zone_t)conv4(native, sp->s_zones);

  /* In V1, the device size was kept in a short, s_nzones, which limited
   * devices to 32K zones.  For V2, it was decided to keep the size as a
   * long.  However, just changing s_nzones to a long would not work, since
   * then the position of s_magic in the super block would not be the same
   * in V1 and V2 file systems, and there would be no way to tell whether
   * a newly mounted file system was V1 or V2.  The solution was to introduce
   * a new variable, s_zones, and copy the size there.
   *
   * Calculate some other numbers that depend on the version here too, to
   * hide some of the differences.
   */
  if (version == V1) {
  	sp->s_block_size = _STATIC_BLOCK_SIZE;
	sp->s_zones = (zone_t) sp->s_nzones;	/* only V1 needs this copy */
	sp->s_inodes_per_block = V1_INODES_PER_BLOCK;
	sp->s_ndzones = V1_NR_DZONES;
	sp->s_nindirs = V1_INDIRECTS;
  } else {
  	if (version == V2)
  		sp->s_block_size = _STATIC_BLOCK_SIZE;
  	if (sp->s_block_size < _MIN_BLOCK_SIZE) {
  		return EINVAL;
	}
	sp->s_inodes_per_block = V2_INODES_PER_BLOCK(sp->s_block_size);
	sp->s_ndzones = V2_NR_DZONES;
	sp->s_nindirs = V2_INDIRECTS(sp->s_block_size);
  }

  /* For even larger disks, a similar problem occurs with s_firstdatazone.
   * If the on-disk field contains zero, we assume that the value was too
   * large to fit, and compute it on the fly.
   */
  if (sp->s_firstdatazone_old == 0) {
	offset = START_BLOCK + sp->s_imap_blocks + sp->s_zmap_blocks;
	offset += (sp->s_ninodes + sp->s_inodes_per_block - 1) /
		sp->s_inodes_per_block;

	sp->s_firstdatazone = (offset + (1 << sp->s_log_zone_size) - 1) >>
		sp->s_log_zone_size;
  } else {
	sp->s_firstdatazone = (zone_t) sp->s_firstdatazone_old;
  }

  if (sp->s_block_size < _MIN_BLOCK_SIZE)
  	return(EINVAL);

  if ((sp->s_block_size % 512) != 0)
  	return(EINVAL);

  if (SUPER_SIZE > sp->s_block_size)
  	return(EINVAL);

  if ((sp->s_block_size % V2_INODE_SIZE) != 0 ||
     (sp->s_block_size % V1_INODE_SIZE) != 0) {
  	return(EINVAL);
  }

  /* Limit s_max_size to LONG_MAX */
  if ((unsigned long)sp->s_max_size > LONG_MAX)
	sp->s_max_size = LONG_MAX;

  sp->s_isearch = 0;		/* inode searches initially start at 0 */
  sp->s_zsearch = 0;		/* zone searches initially start at 0 */
  sp->s_version = version;
  sp->s_native  = native;

  /* Make a few basic checks to see if super block looks reasonable. */
  if (sp->s_imap_blocks < 1 || sp->s_zmap_blocks < 1
				|| sp->s_ninodes < 1 || sp->s_zones < 1
				|| sp->s_firstdatazone <= 4
				|| sp->s_firstdatazone >= sp->s_zones
				|| (unsigned) sp->s_log_zone_size > 4) {
  	printf("not enough imap or zone map blocks, \n");
  	printf("or not enough inodes, or not enough zones, \n"
  		"or invalid first data zone, or zone size too large\n");
	return(EINVAL);
  }
  sp->s_dev = dev;		/* restore device number */
  return(OK);
}

