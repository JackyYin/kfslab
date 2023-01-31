#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> // register_filesystem API
#include <linux/slab.h> // kzalloc

#include "labfs.h"

int labfs_fill_super(struct super_block *sb, void *data, int silent)
{
    int err;
    struct labfs_fs_info *fsi;
    struct inode *inode;
    struct dentry *root;

    fsi = kzalloc(sizeof(struct labfs_fs_info), GFP_KERNEL);
    if (!fsi) {
        err = -ENOMEM;
        goto fail;
    }
    sb->s_fs_info = fsi;

    if (!(inode = labfs_create_dir(sb))) {
        err = -ENOMEM;
        goto fail;
    }

    root = d_make_root(inode);
    if (!root) {
        err = -ENOMEM;
        goto fail;
    }
    sb->s_root = root;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = LABFS_MAGIC;
    sb->s_time_gran = 1;

    return 0;

fail:
    kfree(fsi);
    sb->s_fs_info = NULL;
    return err;
}

struct dentry * labfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    pr_info("labfs_mount invoked...\n");
    /* return mount_single(fs_type, flags, data, labfs_fill_super); */
    return mount_nodev(fs_type, flags, data, labfs_fill_super);
}

void labfs_kill_sb(struct super_block *sb)
{
    struct inode *inode;

    pr_info("labfs_kill_sb invoked...\n");
    spin_lock(&sb->s_inode_list_lock);
    list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
        pr_info("kill inode: %px, ino: %lu, i_count: %d\n", inode, inode->i_ino, atomic_read(&inode->i_count));
        __remove_inode_hash(inode);
    }
    spin_unlock(&sb->s_inode_list_lock);

    return kill_litter_super(sb);
}

struct file_system_type labfs_type = {
    .name = "labfs",
    .mount = labfs_mount,
    .kill_sb = labfs_kill_sb,
    .owner = THIS_MODULE,
    .next = NULL
};

static int __init fs_init(void)
{
    int ret = register_filesystem(&labfs_type);
    if (ret) {
        pr_warn("failed to register fs...\n");
        return ret;
    }

    pr_info("module initiated...\n");
    return 0;
}

static void __exit fs_exit(void)
{
    int ret = unregister_filesystem(&labfs_type);
    if (ret) {
        pr_warn("failed to unregister fs...\n");
    }
    pr_info("module exited...\n");
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("simple lab for a kernel vfs module");
MODULE_AUTHOR("Jacky Yin");

module_init(fs_init);
module_exit(fs_exit);

