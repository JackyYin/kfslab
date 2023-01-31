#ifndef LABFS_H
#define LABFS_H

#define LABFS_MAGIC 0x8B30CA

struct labfs_fs_info {
    atomic_t nr_inode;
};

struct labfs_inode_info {
    atomic_t dir_count;
    unsigned long ino;
    struct list_head child;
    struct list_head subdir;
    spinlock_t subdir_lock;
    unsigned char name[DNAME_INLINE_LEN];
};


extern const struct file_operations labfs_dir_file_ops;

struct inode * labfs_create_dir(struct super_block *sb);
#endif

