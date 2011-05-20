# Echoing into /dev/urandom is needed to prevent read errors. Excuse the fact
# that there are so much of them, but any less wouldn't cut it. What a bloody
# mess.
RANDOMIZE := echo abcedfghijklmnopqrstuvwxyz > /dev/urandom

# In the following comments, "----" is a block and the default 1 block/zone is
# assumed, since there is no way to modify it with /mkfs.mfs.
# Block size is assumed to be 4096 bytes.

all: mntflag
	@echo "You really should pipe the output to less : 'make 2 &>1 | less'"
	@echo "You now have 4 seconds to cancel (C-c) if you haven't."
	@sleep 4
	@$(RANDOMIZE)
	@$(RANDOMIZE)
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file1 bs=4096 count=2 |> /dev/null
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file2 bs=4096 count=1 |> /dev/null
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file3 bs=4096 count=1 |> /dev/null
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file4 bs=4096 count=1 |> /dev/null
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file5 bs=4096 count=1 |> /dev/null
	@$(RANDOMIZE)
	@rm /mnt/disk/file1 /mnt/disk/file3
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file6 bs=4096 count=4 |> /dev/null
	@cp /mnt/disk/file6 file6
################################################################################
# file6     file2  file6  file4  file5  file6
# ---- ---- | ---- | ---- | ---- | ---- | ----
################################################################################
	@echo "--- test common case"
	@echo "expecting 3:"
	@./prog_test defrag /mnt/disk/file6
	@echo "expecting 1:"
	@./prog_test nfrags /mnt/disk/file6
	@echo "no message regarding binary file difference should be displayed"
	@diff /mnt/disk/file6 file6
	@rm file6
################################################################################
# empty     file2  empty  file4  file5  empty  file6
# ---- ---- | ---- | ---- | ---- | ---- | ---- | ---- ---- ---- ----
################################################################################
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file7 bs=4096 count=3 |> /dev/null
	@cp /mnt/disk/file7 file7
	@rm /mnt/disk/file4 /mnt/disk/file5
################################################################################
# file7     file2  file7  empty            file6
# ---- ---- | ---- | ---- | ---- ---- ---- | ---- ---- ---- ----
################################################################################
	@echo "--- test reusing space freed by a defragmentation"
	@./prog_test defrag /mnt/disk/file7
	@echo "expecting 1:"
	@./prog_test nfrags /mnt/disk/file7
	@echo "no message regarding binary file difference should be displayed"
	@diff /mnt/disk/file7 file7
	@rm file7
################################################################################
# empty     file2  empty  file7            file6
# ---- ---- | ---- | ---- | ---- ---- ---- | ---- ---- ---- ----
################################################################################
	@echo "--- test defragmenting a non-existing file"
	@echo "expecting -1:"
	@./prog_test defrag /mnt/disk/file-not-exists
################################################################################
	@echo "--- test defragmenting an empty file"
	@touch /mnt/disk/file9
	@echo "expecting 0:"
	@./prog_test defrag /mnt/disk/file9
	@rm /mnt/disk/file9
################################################################################
	@echo "--- test defragmenting a directory"
	@mkdir /mnt/disk/dir1
	@echo "expecting -1:"
	@./prog_test defrag /mnt/disk/dir1
	@rm -rf /mnt/disk/dir1
################################################################################
# The bit 0 is inaccessible, the bit 1 is taken by the root directory. The first
# block of our schema is then at index 2 and the last at index 12.
	@echo "--- test defragmentation with copy on a bitmap chunk boundary"
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file10 bs=4096 count=4 |> /dev/null
	@cp /mnt/disk/file10 file10
################################################################################
# file10    file2  file10  file7            file6                file10
# ---- ---- | ---- | ---- | ---- ---- ---- | ---- ---- ---- ---- | ----
################################################################################
# The file will be put on a chunk boundary (chunk = 16 bits)
# from bit 14 to bit 17.
	@echo "expecting 3:"
	@./prog_test defrag /mnt/disk/file10
	@echo "no message regarding binary file difference should be displayed"
	@diff /mnt/disk/file10 file10
	@rm file10
################################################################################
# empty     file2  empty  file7            file6                 empty  file10
# ---- ---- | ---- | ---- | ---- ---- ---- | ---- ---- ---- ---- | ---- | ...
################################################################################
# This is to verifiy the file went where we expected it to.
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file11 bs=4096 count=5 |> /dev/null
	@echo "expecting 4:"
	@./prog_test nfrags /mnt/disk/file11
################################################################################
	@echo "there should be no error, 1 directory and 5 files :"
	@/sbin/fsck.mfs /dev/c0d1
	@rm /mnt/disk/*
	@umount /mnt/disk
	@rm mntflag

prog_test : test.c
	$(CC) -o $@ $?

# fsflag is a file whose presence indicate that the fs has been created on
# /dev/c0d1. It is touched when a new filesystem is made.
fsflag:
	/sbin/mkfs.mfs /dev/c0d1 && touch fsflag
	@sleep 2

# mntflag is a file whose presence indicate that the fs on /dev/c0d1 has been
# mounted on /mnt/disk.
mntflag: fsflag
	@mkdir -p /mnt/disk
	@mount /dev/c0d1 /mnt/disk && touch mntflag
	@sleep 2

# Remove the flags (use after or before reboot in case of filesystem
# corruption).
clean:
	@rm -f /mnt/disk/*
	@rm -f mntflag fsflag

.PHONY: all clean