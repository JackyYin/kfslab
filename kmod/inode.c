#include <linux/fs.h> // new_inode
#include <linux/slab.h> // kzalloc
#include <linux/string.h> // memcpy

#include "labfs.h"

static const struct inode_operations labfs_dir_inode_ops;

static struct inode * labfs_iget(struct super_block *sb, unsigned int ino, unsigned long mode)
{
    struct inode *inode;
    struct labfs_inode_info *iinfo;

    inode = new_inode(sb);
    if (!inode)
        return NULL;

    iinfo = kzalloc(sizeof(struct labfs_inode_info), GFP_KERNEL);
    if (!iinfo) {
        iput(inode);
        return NULL;
    }
    pr_info("allocate iinfo: %px, inode: %px, ino: %d\n", iinfo, inode, ino);
    iinfo->ino = ino;
    INIT_LIST_HEAD(&iinfo->subdir);
    spin_lock_init(&iinfo->subdir_lock);

    inode->i_private = iinfo;
    inode->i_ino = ino;
    inode->i_mode = mode;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);

    inode->i_op = &labfs_dir_inode_ops;
    if (S_ISDIR(mode)) {
        inode->i_fop = &labfs_dir_file_ops;
    }
    __insert_inode_hash(inode, ino);

    return inode;
}

static int labfs_new(struct user_namespace *ns,
                           struct inode *dir,
                           struct dentry *dentry,
                           umode_t mode)
{
    struct super_block *sb = dir->i_sb;
    struct labfs_fs_info *fsi = sb->s_fs_info;
    struct inode *inode;
    struct labfs_inode_info *iinfo, *piinfo = (struct labfs_inode_info *)dir->i_private;

    pr_info("labfs_new invoked: %s, piinfo: %px\n", dentry->d_name.name, piinfo);
    inode = labfs_iget(sb, atomic_read(&fsi->nr_inode) + 1, S_IFREG | mode);
    if (!inode)
        return -ENOMEM;

    iinfo = (struct labfs_inode_info *)inode->i_private;
    spin_lock(&iinfo->subdir_lock);
    list_add_tail(&iinfo->child, &piinfo->subdir);
    spin_unlock(&iinfo->subdir_lock);
    memcpy(iinfo->name, dentry->d_iname, DNAME_INLINE_LEN);

    atomic_inc(&piinfo->dir_count);
    atomic_inc(&fsi->nr_inode);
    d_instantiate(dentry, inode);
    return 0;
}

static int labfs_mkdir(struct user_namespace *ns,
                           struct inode *dir,
                           struct dentry *dentry,
                           umode_t mode)
{
    return labfs_new(ns, dir, dentry, mode);
}

static int labfs_create(struct user_namespace *ns,
                           struct inode *dir,
                           struct dentry *dentry,
                           umode_t mode,
                           bool exec)
{
    return labfs_new(ns, dir, dentry, mode);
}

struct dentry *labfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    struct inode *inode = NULL;
    struct labfs_inode_info *iinfo, *diinfo = (struct labfs_inode_info *)dir->i_private;

    pr_info("labfs_lookup invoked: %px, %s\n", dentry, dentry->d_name.name);

    if (dentry == dir->i_sb->s_root) {
        pr_warn("lookup for root dentry...\n");
    }

    spin_lock(&diinfo->subdir_lock);
    list_for_each_entry(iinfo, &diinfo->subdir, child) {
        if (!strncmp(iinfo->name, dentry->d_iname, DNAME_INLINE_LEN)) {
            struct inode *curi = ilookup(dir->i_sb, iinfo->ino);
            if (!curi)
                continue;

            pr_info("found inode: %px, ino: %lu, i_count: %d\n", curi, curi->i_ino, atomic_read(&curi->i_count));
            inode = curi;
            /* iput(inode); */
            break;
        }
    }
    spin_unlock(&diinfo->subdir_lock);

    if (!dentry->d_sb->s_d_op)
        d_set_d_op(dentry, &simple_dentry_operations);
    d_add(dentry, inode);
    /* d_splice_alias(dir, dentry); */
    return NULL;
}

static int labfs_unlink(struct inode *dir, struct dentry *dentry)
{
    struct labfs_inode_info *iinfo, *tmp, *diinfo = (struct labfs_inode_info *)dir->i_private;

    pr_info("labfs_unlink invoked: %px, %s\n", dentry, dentry->d_name.name);

    spin_lock(&diinfo->subdir_lock);
    list_for_each_entry_safe(iinfo, tmp, &diinfo->subdir, child) {
        if (!strncmp(iinfo->name, dentry->d_iname, DNAME_INLINE_LEN)) {
            struct inode *curi = ilookup(dir->i_sb, iinfo->ino);
            if (!curi)
                continue;


            pr_info("found inode: %px, ino: %lu, i_count: %d\n", curi, curi->i_ino, atomic_read(&curi->i_count));
            list_del(&iinfo->child);
            kfree(iinfo);
            atomic_dec(&diinfo->dir_count);
            iput(curi);
            break;
        }
    }
    spin_unlock(&diinfo->subdir_lock);

    return 0;
}

static int labfs_rmdir(struct inode *dir, struct dentry *dentry)
{
    return labfs_unlink(dir, dentry);
}

static const struct inode_operations labfs_dir_inode_ops = {
    .create = labfs_create,
    .lookup = labfs_lookup,
    .unlink = labfs_unlink,
    .mkdir  = labfs_mkdir,
    .rmdir  = labfs_rmdir
};

struct inode * labfs_create_dir(struct super_block *sb)
{
    struct inode *inode;
    struct labfs_fs_info *fsi;

    pr_info("labfs_create_dir invoked...\n");
    fsi = sb->s_fs_info;
    inode = labfs_iget(sb, atomic_read(&fsi->nr_inode) + 1, S_IFDIR | 0644);
    if (!inode)
        return NULL;

    atomic_inc(&fsi->nr_inode);
    return inode;
}
