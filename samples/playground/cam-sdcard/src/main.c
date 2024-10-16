/*
 * Copyright (c) 2019 Tavish Naruka <tavishnaruka@gmail.com>
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample which uses the filesystem API and SDHC driver */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

#include "fs_operations.h"
#include "cam_operations.h"

LOG_MODULE_REGISTER(main);

#define MAX_PATH 128
#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/"DISK_DRIVE_NAME":"

static const char *disk_drive_name = DISK_DRIVE_NAME;
static const char *disk_mount_pt = DISK_MOUNT_PT;

static int shell_lsdir(const struct shell *sh, size_t argc, char **argv)
{
    int res = fs_op_lsdir(DISK_MOUNT_PT);
    if (res != 0) {
        LOG_ERR("Error showing dir's content");
    }

	return 0;
}

static int shell_umount(const struct shell *sh, size_t argc, char **argv)
{
    fs_op_deinit();

    LOG_INF("umount done");

	return 0;
}

SHELL_CMD_REGISTER(lsdir, NULL, "ls dir", shell_lsdir);
SHELL_CMD_REGISTER(umount, NULL, "umount dir", shell_umount);

#define SAMPLE_FNAME "frame-"
#define CHUNK_SIZE 64

int write_frame(int num, char *buffer, size_t len)
{
    static char path[MAX_PATH];
    struct fs_file_t file;
    int ret;
    int base = strlen(disk_mount_pt);
	fs_file_t_init(&file);
    strncpy(path, disk_mount_pt, sizeof(path));

    path[base++] = '/';
    path[base] = 0;
    // strcat(&path[base], SAMPLE_FNAME);
    snprintf(&path[base], sizeof(path) - base, "%s%d", SAMPLE_FNAME, num);

	if (fs_open(&file, path, FS_O_CREATE | FS_O_WRITE)) {
		LOG_ERR("Failed to create file %s", path);
		return -1;
	}

    ret = fs_truncate(&file, len);
    if (ret) {
        LOG_ERR("Failed to truncate file");
        fs_close(&file);
        return ret;
    }

    ret = fs_seek(&file, 0, FS_SEEK_SET);
    if (ret) {
        LOG_ERR("Failed to seek to start");
        return -1;
    }

    /* Full write */
    LOG_INF("writing file: size: %u", len);
    ret = fs_write(&file, buffer, len);
    if (ret < 0) {
        LOG_ERR("Cannot write file");
	    fs_close(&file);
        return ret;
    }

    if (ret != len) {
        LOG_ERR("errno: %d", errno);
        fs_close(&file);
        return ret;
    }
    /* Full write end */

    fs_sync(&file);
	fs_close(&file);

    return 0;
}


int grab_and_save(struct camera *video, int num)
{
    int ret;
    struct video_buffer *vbuf;

    ret = camera_frame_get(video, &vbuf);
    if (ret) {
        return ret;
    }
    LOG_INF("camera got frame, ptr %p!", vbuf);

    camera_stream_stop(video);

    ret = write_frame(num, vbuf->buffer, vbuf->bytesused);
    if (ret) {
        return ret;
    }
    
    camera_setup_buffers(video);
    camera_stream_start(video);

    // ret = camera_frame_put(video, vbuf);
    // if (ret) {
    //     LOG_ERR("Unable to requeue video buf");
    //     return ret;
    // }

    LOG_INF("Done!");

    return 0;
}

int grab_and_do_nothing(struct camera *video)
{
    int ret;
    struct video_buffer *vbuf;

    ret = camera_frame_get(video, &vbuf);
    if (ret) {
        LOG_ERR("Unable to dequeue video buf");
        return ret;
    }

    ret = camera_frame_put(video, vbuf);
    if (ret) {
        LOG_ERR("Unable to requeue video buf");
        return ret;
    }

    return 0;
}

#define MKFS_FS_TYPE FS_FATFS
#define MKFS_DEV_ID "SD:"
#define MKFS_FLAGS 0

int main()
{
	unsigned int frame = 0;
    struct camera *video = k_malloc(sizeof(struct camera));
    memset(video, 0, sizeof(*video));
    int ret;
    int i;

    ret = fs_op_init(disk_drive_name, disk_mount_pt);
    if (ret) {
        return ret;
    }

    k_sleep(K_MSEC(1000));
    LOG_INF("mounted drive!");

    ret = camera_init(video);
    if (ret) {
        return ret;
    }
    LOG_INF("camera init done!");

    ret = camera_setup_buffers(video);
    if (ret) {
        return ret;
    }

    ret = camera_stream_start(video);
    if (ret) {
        return ret;
    }
    LOG_INF("camera stream started!");


    // for (i = 0; i < 10; i++) {
    if (grab_and_save(video, 2)) {
        LOG_ERR("Cannot grab and save");
        k_sleep(K_MSEC(1000));
        return -1;
    }
    // }
    camera_stream_stop(video);
    k_free(video);

    LOG_INF("done");
}