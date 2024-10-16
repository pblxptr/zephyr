#pragma once

#include <zephyr/drivers/video.h>

struct camera {
    const struct device *dev;
	struct video_buffer *buffers[2]; //CONFIG_VIDEO_BUFFER_POOL_NUM_MAX
};

int camera_init(struct camera *video);
int camera_setup_buffers(struct camera *video);
int camera_stream_start(struct camera *video);
int camera_stream_stop(struct camera *video);
int camera_frame_get(struct camera *video, struct video_buffer **vbuf);
int camera_frame_put(struct camera *video, struct video_buffer *buffer);