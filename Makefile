RANDOMIZE := echo abcedfghijklmnopqrstuvwxyz > /dev/urandom

all:
	@echo "run all tests"

.PHONY: all nfrags defrag fsck clean mrproper wipe files

# After execution of this script, the file layout will be:
# file6     file2  file6  file4  file5  file6
# ---- ---- | ---- | ---- | ---- | ---- | ----
# (where "----" is a block and the default 1 block/zone is assumed)
nfrags: prog_test files
	./prog_test 1

defrag: prog_test files
	./prog_test 2

prog_test : test.c
	$(CC) -o $@ $?

# Echoing into /dev/urandom is needed to prevent read errors. Excuse the fact
# that there are so much of them, but any less wouldn't cut it. What a bloody
# mess.
files: mntflag
	@$(RANDOMIZE)
	@$(RANDOMIZE)
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file1 bs=4096 count=2
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file2 bs=4096 count=1
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file3 bs=4096 count=1
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file4 bs=4096 count=1
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file5 bs=4096 count=1
	@$(RANDOMIZE)
	@rm /mnt/disk/file1 /mnt/disk/file3
	@$(RANDOMIZE)
	@dd if=/dev/urandom of=/mnt/disk/file6 bs=4096 count=4
	@sleep 2

# File system check + file usage on /mnt/disk.
fsck:
	{ /sbin/fsck.mfs /dev/c0d1 && du /mnt/disk/*; } | less

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

# Delete all content from /mnt/disk and unmount it.
clean:
	@rm -f /mnt/disk/*
	@umount /mnt/disk && rm mntflag

# Remove the flags (use after or before reboot in case of filesystem
# corruption).
wipe:
	@rm -f /mnt/disk/*
	@rm -f mntflag fsflag

# clean + remove fsflag (filesystem must be rebuild).
mrproper: clean
	@rm fsflag
