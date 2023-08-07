#ifndef CAMERA_100ASK_DEV_H
#define CAMERA_100ASK_DEV_H

#include <config.h>
#include <disp_manager.h>
#include <video_manager.h>
#include <convert_manager.h>
#include <render.h>

typedef enum {
    CAMERA_100ASK_OPT_NONE = 0,
    CAMERA_100ASK_OPT_UPDATE_BRIGHTNESS,
    CAMERA_100ASK_OPT_TAKE_PHOTOS,
    CAMERA_100ASK_OPT_TAKE_VIDEO,
} camera_100ask_opt_t;

int camera_100ask_dev_init(char * dev);

void camera_100ask_dev_set_brightness(int value);

PT_VideoBuf camera_100ask_dev_get_video_buf_cur(void);

#endif /*CAMERA_100ASK_DEV_H*/
