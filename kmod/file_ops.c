#include <linux/fs.h> // struct file_operations

#include "labfs.h"

static int labfs_readdir(struct file *file, struct dir_context *ctx)
{
    struct inode *curi, *inode = file_inode(file);
    struct labfs_inode_info *iinfo, *tmp, *diinfo = (struct labfs_inode_info *)inode->i_private;

    /* pr_info("labfs_readdir invoked..., pos: %d\n", ctx->pos); */

    if (ctx->pos >= atomic_read(&diinfo->dir_count))
        return 0;

    spin_lock(&diinfo->subdir_lock);
    list_for_each_entry_safe(iinfo, tmp, &diinfo->subdir, child) {
        if ((curi = ilookup(inode->i_sb, iinfo->ino))) {
            pr_info("labfs_readdir get inode: %px, ino: %lu, i_count: %d\n", curi, curi->i_ino, atomic_read(&curi->i_count));

            if (S_ISDIR(curi->i_mode))
                dir_emit(ctx, iinfo->name, DNAME_INLINE_LEN, curi->i_ino, DT_DIR);
            else if (S_ISREG(curi->i_mode))
                dir_emit(ctx, iinfo->name, DNAME_INLINE_LEN, curi->i_ino, DT_REG);
            else
                dir_emit(ctx, iinfo->name, DNAME_INLINE_LEN, curi->i_ino, DT_UNKNOWN);
            ctx->pos++;
            iput(curi);
        }
    }
    spin_unlock(&diinfo->subdir_lock);
    return 0;
}

const struct file_operations labfs_dir_file_ops = {
    /* .open = simple_open */
	.iterate_shared	= labfs_readdir
};
