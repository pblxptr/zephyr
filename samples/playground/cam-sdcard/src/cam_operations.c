

#include "cam_operations.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(main);

int camera_init(struct camera *video)
{
	struct video_format fmt;
	struct video_frmival frmival;
	struct video_frmival_enum fie;
	struct video_caps caps;
    size_t bsize;
	int i;

	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));

    if (!device_is_ready(dev)) {
		LOG_ERR("%s: video device not ready.", video->dev->name);
		return 0;
	}
    video->dev = dev;


	LOG_INF("Video device: %s", video->dev->name);

	/* Get capabilities */
	if (video_get_caps(video->dev, VIDEO_EP_OUT, &caps)) {
		LOG_ERR("Unable to retrieve video capabilities");
		return 0;
	}

	printk("- Capabilities:\n");
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* fourcc to string */
		LOG_INF("  %c%c%c%c width [%u; %u; %u] height [%u; %u; %u]",
		       (char)fcap->pixelformat, (char)(fcap->pixelformat >> 8),
		       (char)(fcap->pixelformat >> 16), (char)(fcap->pixelformat >> 24),
		       fcap->width_min, fcap->width_max, fcap->width_step, fcap->height_min,
		       fcap->height_max, fcap->height_step);
		i++;
	}
	

	/* Get default/native format */
	if (video_get_format(video->dev, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return -1;
	}

	LOG_INF("- Default format: %c%c%c%c %ux%u", (char)fmt.pixelformat,
	       (char)(fmt.pixelformat >> 8), (char)(fmt.pixelformat >> 16),
	       (char)(fmt.pixelformat >> 24), fmt.width, fmt.height);

	if (!video_get_frmival(video->dev, VIDEO_EP_OUT, &frmival)) {
		LOG_ERR("- Default frame rate : %f fps\n",
		       1.0 * frmival.denominator / frmival.numerator);
	}

	LOG_INF("- Supported frame intervals for the default format:");
	memset(&fie, 0, sizeof(fie));
	fie.format = &fmt;
	while (video_enum_frmival(video->dev, VIDEO_EP_OUT, &fie) == 0) {
		if (fie.type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
			LOG_INF("   %u/%u ", fie.discrete.numerator, fie.discrete.denominator);
		} else {
			LOG_INF("   [min = %u/%u; max = %u/%u; step = %u/%u]",
			       fie.stepwise.min.numerator, fie.stepwise.min.denominator,
			       fie.stepwise.max.numerator, fie.stepwise.max.denominator,
			       fie.stepwise.step.numerator, fie.stepwise.step.denominator);
		}
		fie.index++;
	}

	/* Apply format from config */
	#if CONFIG_VIDEO_FORMAT_HEIGHT
		fmt.height = CONFIG_VIDEO_FORMAT_HEIGHT;
	#endif

	#if CONFIG_VIDEO_FORMAT_WIDTH
		fmt.width = CONFIG_VIDEO_FORMAT_WIDTH;
		fmt.pitch = fmt.width * 2;
	#endif

	if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "")) {
		fmt.pixelformat =
			video_fourcc(CONFIG_VIDEO_PIXEL_FORMAT[0], CONFIG_VIDEO_PIXEL_FORMAT[1],
				     CONFIG_VIDEO_PIXEL_FORMAT[2], CONFIG_VIDEO_PIXEL_FORMAT[3]);
	}

	LOG_INF("- Video format: %c%c%c%c %ux%u", (char)fmt.pixelformat,
	       (char)(fmt.pixelformat >> 8), (char)(fmt.pixelformat >> 16),
	       (char)(fmt.pixelformat >> 24), fmt.width, fmt.height);

	if (video_set_format(video->dev, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to set format");
		return 0;
	}
	LOG_INF("Format set");

	bsize = fmt.pitch * fmt.height; // For 240x240 = 115200

    // Allocate buffers 
	for (i = 0; i < ARRAY_SIZE(video->buffers); i++) {
		video->buffers[i] = video_buffer_aligned_alloc(bsize, CONFIG_VIDEO_BUFFER_POOL_ALIGN);
		if (video->buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return -1;
		}
	}

    return 0;
}

int camera_setup_buffers(struct camera *video)
{
	int ret;
	int i;
    for (i = 0; i < ARRAY_SIZE(video->buffers); i++) {
        LOG_INF(" - equeue buffer");
        ret = video_enqueue(video->dev, VIDEO_EP_OUT, video->buffers[i]);
		if (ret) {
			LOG_ERR("Cannot enqueue buffer");
			return ret;
		}
	}
	return 0;
}

int camera_stream_start(struct camera *video)
{
    /* Start video capture */
    if (video_stream_start(video->dev)) {
        LOG_ERR("Unable to start video");
        return -1;
    }

    return 0;
}

int camera_stream_stop(struct camera *video)
{
    video_stream_stop(video->dev);

	return 0;
}

int camera_frame_get(struct camera *video, struct video_buffer **vbuf)
{
    int ret = video_dequeue(video->dev, VIDEO_EP_OUT, vbuf, K_MSEC(5000));
    if (ret) {
        LOG_ERR("Unable to dequeue video buf");
        return -1;
    }
    return 0;
}

int camera_frame_put(struct camera *video, struct video_buffer *vbuf)
{
    int ret = video_enqueue(video->dev, VIDEO_EP_OUT, vbuf);
    if (ret) {
        LOG_ERR("Unable to enqueue video buf");
        return -1;
    }
    return 0;
}