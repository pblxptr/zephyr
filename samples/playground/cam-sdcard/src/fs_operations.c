
#include "fs_operations.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(main);

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#else
#error "CONFIG_FAT_FILESYSTEM_ELM must be specified."
#endif

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};
#define FS_RET_OK FR_OK



int fs_op_init(const char *disk_drive_name, const char* mnt_pt)
{
	/* raw disk i/o */
	do {
		uint64_t memory_size_mb;
		uint32_t block_count;
		uint32_t block_size;

		if (disk_access_ioctl(disk_drive_name,
				DISK_IOCTL_CTRL_INIT, NULL) != 0) {
			LOG_ERR("Storage init ERROR!");
			break;
		}

		if (disk_access_ioctl(disk_drive_name,
				DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
			LOG_ERR("Unable to get sector count");
			break;
		}

		if (disk_access_ioctl(disk_drive_name,
				DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
			LOG_ERR("Unable to get sector size");
			break;
		}

		memory_size_mb = (uint64_t)block_count * block_size;

		if (disk_access_ioctl(disk_drive_name,
				DISK_IOCTL_CTRL_DEINIT, NULL) != 0) {
			LOG_ERR("Storage deinit ERROR!");
			break;
		}

	    LOG_INF("Block count %u", block_count);
		LOG_INF("Sector size %u", block_size);
		LOG_INF("Memory Size(MB) %u", (uint32_t)(memory_size_mb >> 20));
	} while (0);

	mp.mnt_point = mnt_pt;

	int res = fs_mount(&mp);
	if (res == FS_RET_OK) {
		LOG_INF("Disk mounted");
		/* Try to unmount and remount the disk */
		res = fs_unmount(&mp);
		if (res != FS_RET_OK) {
			LOG_ERR("Error unmounting disk");
			return res;
		}
		res = fs_mount(&mp);
		if (res != FS_RET_OK) {
			LOG_ERR("Error remounting disk");
			return res;
		}
	} else {
		LOG_ERR("Error mounting disk");
        return res;
	}
    return FS_RET_OK;
}

int fs_op_lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	int count = 0;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		printk("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	printk("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("[DIR ] %s\n", entry.name);
		} else {
			printk("[FILE] %s (size = %zu)\n",
				entry.name, entry.size);
		}
		count++;
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);
	if (res == 0) {
		res = count;
	}

	return res;
}

int fs_op_deinit(void)
{
	int res = fs_unmount(&mp);
    if (res != FS_RET_OK) {
        LOG_ERR("Error unmouting the disk");
    }
    return res;
}