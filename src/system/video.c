/**
 * @file video.c
 * @brief Implements Neo Geo video initialization and refresh-rate query.
 */

#include "system/video.h"

#include <ngdevkit/registers.h>

static u8 video_refresh_rate;

/** Detects the active Neo Geo video refresh rate used by frame-based timing. */
static u8 video_detect_refresh_rate(void) {
    if ((*REG_LSPCMODE & 0x0008u) != 0u) {
        return UNSIGNED_VIDEO_MODE_50HZ;
    }

    return UNSIGNED_VIDEO_MODE_60HZ;
}

void unsigned_video_init(void) {
    video_refresh_rate = video_detect_refresh_rate();
}

u8 unsigned_video_get_refresh_rate(void) {
    return video_refresh_rate;
}
