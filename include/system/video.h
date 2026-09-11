/**
 * @file video.h
 * @brief Neo Geo video initialization and refresh-rate query.
 */

#ifndef UNSIGNED_SYSTEM_VIDEO_H
#define UNSIGNED_SYSTEM_VIDEO_H

#include "core/types.h"

#define UNSIGNED_VIDEO_MODE_50HZ 50
#define UNSIGNED_VIDEO_MODE_60HZ 60

#define UNSIGNED_VIDEO_SCB3_Y_ORIGIN 496

/**
 * @brief Initializes the video to a valid empty runtime state.
 */
void unsigned_video_init(void);

/**
 * @brief Returns refresh rate from the video.
 * @return Detected frame rate in hertz (`UNSIGNED_VIDEO_MODE_50HZ` or `UNSIGNED_VIDEO_MODE_60HZ`) after video initialization.
 */
u8 unsigned_video_get_refresh_rate(void);

#endif
