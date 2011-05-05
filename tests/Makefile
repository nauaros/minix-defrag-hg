all:
    @echo "some helpful advice"

nfrags: counting

# After execution of this script, the file layout will be:
# file1     file6  file3  file6  file5
# ---- ---- | ---- | ---- | ---- | ---- | ----
# (where "-" is a block and the default 4 blocks/zone is assumed)
counting: test
    /sbin/mkfs.mfs /dev/c0d1
    mount /dev/c0d1 /mnt/disk
    @dd if = /dev/urandom of = /mnt/disk/file1 count = 8
    @dd if = /dev/urandom of = /mnt/disk/file2 count = 4
    @dd if = /dev/urandom of = /mnt/disk/file3 count = 4
    @dd if = /dev/urandom of = /mnt/disk/file4 count = 4
    @dd if = /dev/urandom of = /mnt/disk/file5 count = 4
    @dd rm /mnt/disk/file2 /mnt/disk/file4
    @dd if = /dev/urandom of = /mnt/disk/file6 count = 12
    @test 1

test : test.c

.PHONY: all nfrags defrag counting