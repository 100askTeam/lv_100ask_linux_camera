#include "camera_100ask_ui.h"
#include "camera_100ask_dev.h"
#include <pthread.h>

#define PATH_FILE_NAME_LEN 256
#define BLINK_TIME         200 /*ms*/
#define BOOT_TIME          1500

typedef struct{
	uint8_t * name;            // 蛇身
} photo_file_t;

static uint8_t g_dir_path[PATH_FILE_NAME_LEN];
static lv_img_dsc_t * img_dsc;
static photo_file_t * g_node_ll;
static lv_ll_t photo_file_ll;
static lv_obj_t * g_obj_blink;
static lv_obj_t * g_slider_label_setting;
static lv_obj_t * g_img_photo_browser;

extern pthread_mutex_t g_camera_100ask_mutex;
static void lv_100ask_boot_animation(uint32_t boot_time);
static void lv_100ask_boot_animation(uint32_t boot_time);
static void camera_startup_timer(lv_timer_t * timer);
static void camera_work_timer(lv_timer_t * timer);
static void blink_timer(lv_timer_t * timer);

static void btn_capture_event_handler(lv_event_t * e);
static void btn_setting_event_handler(lv_event_t * e);
static void slider_setting_event_cb(lv_event_t * e);

static void btn_photo_browser_event_handler(lv_event_t * e);
static void btn_open_photo_browser_event_handler(lv_event_t * e);

static bool is_end_with(const char * str1, const char * str2);


void camera_100ask_ui_init(void)
{
    lv_100ask_boot_animation(BOOT_TIME);

    lv_obj_t * cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_y(cont, 0);

    lv_obj_fade_in(cont, 0, BOOT_TIME);

    img_dsc = lv_mem_alloc(sizeof(lv_img_dsc_t));
    lv_memset_00(img_dsc, sizeof(lv_img_dsc_t)); 

    lv_obj_t * img = lv_img_create(cont);
    lv_img_set_antialias(img, true);
    lv_obj_center(img);
    lv_timer_t * timer = lv_timer_create(camera_startup_timer, BOOT_TIME, img);
    lv_timer_set_repeat_count(timer, 1);

    /*Blinking effect*/
    g_obj_blink = lv_obj_create(cont);
    lv_obj_set_style_border_width(g_obj_blink, 0, 0);
    lv_obj_set_style_pad_all(g_obj_blink, 0, 0);
    lv_obj_set_style_radius(g_obj_blink, 0, 0);
    lv_obj_set_size(g_obj_blink, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(g_obj_blink, lv_color_hex(0x000000), 0);
    lv_obj_add_flag(g_obj_blink, LV_OBJ_FLAG_HIDDEN);


    /*btn_capture*/
    static lv_style_t style;
    lv_style_init(&style);

    lv_style_set_radius(&style, LV_RADIUS_CIRCLE);

    lv_style_set_bg_opa(&style, LV_OPA_100);
    lv_style_set_bg_color(&style, lv_color_hex(0xffffff));

    lv_style_set_border_opa(&style, LV_OPA_40);
    lv_style_set_border_width(&style, 2);
    lv_style_set_border_color(&style, lv_color_hex(0x000000));

    lv_style_set_outline_opa(&style, LV_OPA_COVER);
    lv_style_set_outline_color(&style, lv_color_hex(0x000000));

    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_pad_all(&style, 10);

    /*Init the pressed style*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);

    /*Ad a large outline when pressed*/
    lv_style_set_outline_width(&style_pr, 15);
    lv_style_set_outline_opa(&style_pr, LV_OPA_TRANSP);

    lv_style_set_translate_y(&style_pr, 5);
    //lv_style_set_shadow_ofs_y(&style_pr, 3);
    lv_style_set_bg_color(&style_pr, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_color(&style_pr, lv_palette_main(LV_PALETTE_GREEN));

    /*Add a transition to the the outline*/
    static lv_style_transition_dsc_t trans;
    static lv_style_prop_t props[] = {LV_STYLE_OUTLINE_WIDTH, LV_STYLE_OUTLINE_OPA, 0};
    lv_style_transition_dsc_init(&trans, props, lv_anim_path_linear, 300, 0, NULL);
    lv_style_set_transition(&style_pr, &trans);

    lv_obj_t * cont_capture = lv_obj_create(cont);
    lv_obj_set_size(cont_capture, 100, 100);
    lv_obj_set_align(cont_capture, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(cont_capture, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(cont_capture, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(cont_capture, 0, 0);

    lv_obj_t * btn_capture = lv_btn_create(cont_capture);
    lv_obj_set_size(btn_capture, 75, 75);
    lv_obj_set_align(btn_capture, LV_ALIGN_CENTER);

    lv_obj_add_style(btn_capture, &style, 0);
    lv_obj_add_style(btn_capture, &style_pr, LV_STATE_PRESSED);

    lv_obj_add_event_cb(btn_capture, btn_capture_event_handler, LV_EVENT_ALL, NULL);

    /*camera setting*/
    lv_obj_t * btn_setting = lv_btn_create(cont);
    lv_obj_set_style_radius(btn_setting, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(btn_setting, 50, 50);
    lv_obj_align_to(btn_setting, cont_capture, LV_ALIGN_OUT_RIGHT_MID, (LV_VER_RES / 4), 0);

    lv_obj_t * label_setting = lv_label_create(btn_setting);
    lv_obj_set_style_text_font(label_setting, &lv_font_montserrat_28, 0);
    lv_label_set_text(label_setting, LV_SYMBOL_SETTINGS);
    lv_obj_set_align(label_setting, LV_ALIGN_CENTER);

    // slider setting
    lv_obj_t * slider_setting = lv_slider_create(cont);
    lv_slider_set_mode(slider_setting, LV_SLIDER_MODE_SYMMETRICAL);
    lv_slider_set_range(slider_setting, -255, 255);
    lv_obj_add_flag(slider_setting, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align_to(slider_setting, btn_setting, LV_ALIGN_OUT_TOP_MID, 0, -10);

    /*Create a label below the slider*/
    g_slider_label_setting = lv_label_create(cont);
    lv_label_set_text(g_slider_label_setting, "camera brightness:0");
    lv_obj_add_flag(g_slider_label_setting, LV_OBJ_FLAG_HIDDEN);

    lv_obj_align_to(g_slider_label_setting, slider_setting, LV_ALIGN_OUT_TOP_MID, 0, -10);

    lv_obj_add_event_cb(btn_setting, btn_setting_event_handler, LV_EVENT_CLICKED, slider_setting);
    lv_obj_add_event_cb(slider_setting, slider_setting_event_cb, LV_EVENT_VALUE_CHANGED, g_slider_label_setting);

    /*Photo Browser*/
    lv_obj_t * btn_open_photo_browser = lv_btn_create(cont);
    lv_obj_set_style_radius(btn_open_photo_browser, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(btn_open_photo_browser, 50, 50);
    lv_obj_align_to(btn_open_photo_browser, cont_capture, LV_ALIGN_OUT_LEFT_MID, -(LV_VER_RES / 4), 0);

    lv_obj_t * label_photo_browser = lv_label_create(btn_open_photo_browser);
    lv_obj_set_style_text_font(label_photo_browser, &lv_font_montserrat_28, 0);
    lv_label_set_text(label_photo_browser, LV_SYMBOL_IMAGE);
    lv_obj_set_align(label_photo_browser, LV_ALIGN_CENTER);

    lv_obj_t * cont_photo_browser = lv_obj_create(cont);
    lv_obj_set_size(cont_photo_browser, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont_photo_browser, 0, 0);
    lv_obj_set_style_radius(cont_photo_browser, 0, 0);
    lv_obj_set_align(cont_photo_browser, LV_ALIGN_CENTER);
    lv_obj_add_flag(cont_photo_browser, LV_OBJ_FLAG_HIDDEN);

    g_img_photo_browser = lv_img_create(cont_photo_browser);
    //lv_img_set_src(g_img_photo_browser, "//mnt/100ask-picture-20230803-105727.bmp");
    //lv_img_set_src(g_img_photo_browser, "//mnt/12345.bmp");
    //lv_img_set_src(g_img_photo_browser, "//mnt/test.png");
    lv_obj_set_align(g_img_photo_browser, LV_ALIGN_CENTER);

    lv_obj_t * btn_photo_browser_pre = lv_btn_create(cont_photo_browser);
    lv_obj_set_style_radius(btn_photo_browser_pre, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(btn_photo_browser_pre, 50, 50);
    lv_obj_align(btn_photo_browser_pre, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t * label_photo_browser_pre = lv_label_create(btn_photo_browser_pre);
    lv_obj_set_style_text_font(label_photo_browser_pre, &lv_font_montserrat_28, 0);
    lv_label_set_text(label_photo_browser_pre, LV_SYMBOL_LEFT);
    lv_obj_set_align(label_photo_browser_pre, LV_ALIGN_CENTER);

    lv_obj_t * btn_photo_browser_next = lv_btn_create(cont_photo_browser);
    lv_obj_set_style_radius(btn_photo_browser_next, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(btn_photo_browser_next, 50, 50);
    lv_obj_align(btn_photo_browser_next, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t * label_photo_browser_next = lv_label_create(btn_photo_browser_next);
    lv_obj_set_style_text_font(label_photo_browser_next, &lv_font_montserrat_28, 0);
    lv_label_set_text(label_photo_browser_next, LV_SYMBOL_RIGHT);
    lv_obj_set_align(label_photo_browser_next, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(btn_photo_browser_pre, btn_photo_browser_event_handler, LV_EVENT_CLICKED, label_photo_browser_pre);
    lv_obj_add_event_cb(btn_photo_browser_next, btn_photo_browser_event_handler, LV_EVENT_CLICKED, label_photo_browser_next);
    lv_obj_add_event_cb(btn_open_photo_browser, btn_open_photo_browser_event_handler, LV_EVENT_CLICKED, cont_photo_browser);

}

static void lv_100ask_boot_animation(uint32_t boot_time)
{
    LV_IMG_DECLARE(img_lv_100ask_demo_logo);
    lv_obj_t * logo = lv_img_create(lv_scr_act());
    lv_img_set_src(logo, &img_lv_100ask_demo_logo);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, 0);

    /*Animate in the content after the intro time*/
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_path_cb(&a, lv_anim_path_bounce);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_set_var(&a, logo);
    lv_anim_set_time(&a, boot_time);
    lv_anim_set_delay(&a, 0);
    lv_anim_set_values(&a, 1, LV_IMG_ZOOM_NONE);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t) lv_img_set_zoom);
	lv_anim_set_ready_cb(&a, lv_obj_del_anim_ready_cb);
    lv_anim_start(&a);

    /* Create an intro from a label */
    lv_obj_t * title = lv_label_create(lv_scr_act());
    //lv_label_set_text(title, "100ASK LVGL DEMO\nhttps://www.100ask.net\nhttp:/lvgl.100ask.net");
	lv_label_set_text(title, "100ASK LVGL DEMO");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, LV_STATE_DEFAULT); // Please enable LV_FONT_MONTSERRAT_22 in lv_conf.h
    lv_obj_set_style_text_line_space(title, 8, LV_STATE_DEFAULT);
    lv_obj_align_to(title, logo, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_obj_fade_out(title, 0, boot_time);
    lv_obj_fade_out(logo, 0, boot_time);
}


static void camera_startup_timer(lv_timer_t * timer)
{
    lv_obj_t * img = (lv_obj_t *)timer->user_data;
    lv_timer_create(camera_work_timer, 0, img);
}

static void camera_work_timer(lv_timer_t * timer)
{
    /*Use the user_data*/
    lv_obj_t * img = (lv_obj_t *)timer->user_data;
    
    /*Do something with LVGL*/
    pthread_mutex_lock(&g_camera_100ask_mutex);
    PT_VideoBuf VideoBufCur = camera_100ask_dev_get_video_buf_cur();
    PT_PixelDatas ptSmallPic = &VideoBufCur->tPixelDatas;
    img_dsc->data = ptSmallPic->aucPixelDatas;
    img_dsc->data_size = (VideoBufCur->tPixelDatas.iBpp / 8) * VideoBufCur->tPixelDatas.iWidth * VideoBufCur->tPixelDatas.iHeight;

    img_dsc->header.w = VideoBufCur->tPixelDatas.iWidth;
    img_dsc->header.h = VideoBufCur->tPixelDatas.iHeight;
    img_dsc->header.cf = LV_IMG_CF_TRUE_COLOR;
    lv_img_set_src(img, img_dsc);

    pthread_mutex_unlock(&g_camera_100ask_mutex); 
  
}

static void blink_timer(lv_timer_t * timer)
{
    lv_obj_add_flag(g_obj_blink, LV_OBJ_FLAG_HIDDEN);
}

static void btn_capture_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn_capture = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        camera_100ask_dev_set_opt(CAMERA_100ASK_OPT_TAKE_PHOTOS);

        lv_obj_clear_flag(g_obj_blink, LV_OBJ_FLAG_HIDDEN);

        lv_timer_t * timer = lv_timer_create(blink_timer, BLINK_TIME, NULL);
        lv_timer_set_repeat_count(timer, 1);
    }
}


static void btn_setting_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * slider_setting = lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        if(lv_obj_has_flag(slider_setting, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_clear_flag(slider_setting, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(g_slider_label_setting, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(slider_setting, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_slider_label_setting, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void slider_setting_event_cb(lv_event_t * e)
{
    lv_obj_t * slider_setting = lv_event_get_target(e);
    lv_obj_t * slider_label_setting  = lv_event_get_user_data(e);

    int slider_value = (int)lv_slider_get_value(slider_setting);

    char buf[32];
    lv_snprintf(buf, sizeof(buf), "camera brightness: %d", slider_value);
    lv_label_set_text(slider_label_setting, buf);
    //lv_obj_align_to(slider_label_setting, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);

    camera_100ask_dev_set_brightness(slider_value);
    camera_100ask_dev_set_opt(CAMERA_100ASK_OPT_UPDATE_BRIGHTNESS);
}


static void btn_open_photo_browser_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * cont_photo_browse = lv_event_get_user_data(e);

    char file_path_name[PATH_FILE_NAME_LEN];

    if(code == LV_EVENT_CLICKED) {
        lv_obj_move_foreground(btn);
        if(lv_obj_has_flag(cont_photo_browse, LV_OBJ_FLAG_HIDDEN))
        {
            _lv_ll_init(&photo_file_ll, sizeof(photo_file_t));

            lv_snprintf(g_dir_path, sizeof(g_dir_path), "/%s", getcwd(NULL, 0));

            lv_fs_dir_t dir;
            lv_fs_res_t res;
            res = lv_fs_dir_open(&dir, g_dir_path);
            if(res != LV_FS_RES_OK) {
                LV_LOG_USER("Open dir error %d!", res);
                return;
            }

            char fn[PATH_FILE_NAME_LEN];
            photo_file_t * node_ll;
            while(1) {
                res = lv_fs_dir_read(&dir, fn);
                if(res != LV_FS_RES_OK) {
                    LV_LOG_USER("Driver, file or directory is not exists %d!", res);
                    break;
                }

                /*fn is empty, if not more files to read*/
                if(strlen(fn) == 0) {
                    LV_LOG_USER("Not more files to read!");
                    break;
                }

                // 识别并文件
                if ((is_end_with(fn, ".png") == true)  || (is_end_with(fn, ".PNG") == true)  ||\
                    (is_end_with(fn , ".jpg") == true) || (is_end_with(fn , ".JPG") == true) ||\
                    (is_end_with(fn , ".sjpg") == true) || (is_end_with(fn , ".SJPG") == true) ||\
                    (is_end_with(fn , ".bmp") == true) || (is_end_with(fn , ".BMP") == true))
                {
                    node_ll = _lv_ll_ins_tail(&photo_file_ll);
                    node_ll->name = lv_mem_alloc(strlen(fn));
                    strcpy(node_ll->name, fn);
                    LV_LOG_USER("%s", node_ll->name);
                }
            }

            lv_fs_dir_close(&dir);

            g_node_ll = _lv_ll_get_tail(&photo_file_ll);  // r
            lv_snprintf(file_path_name, sizeof(file_path_name), "%s/%s", g_dir_path, g_node_ll->name);
            lv_img_set_src(g_img_photo_browser, file_path_name);

            lv_obj_clear_flag(cont_photo_browse, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            photo_file_t * node_ll = _lv_ll_get_head(&photo_file_ll);
            while(node_ll != NULL)
            {
                lv_mem_free(node_ll->name);
                node_ll = _lv_ll_get_next(&photo_file_ll, node_ll);
            }
            _lv_ll_clear(&photo_file_ll);

            g_node_ll = NULL;

            lv_obj_add_flag(cont_photo_browse, LV_OBJ_FLAG_HIDDEN);
        }
    }
}


static void btn_photo_browser_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_event_get_user_data(e);
    photo_file_t * tmp_node_ll;
    char file_path_name[PATH_FILE_NAME_LEN];

    if(code == LV_EVENT_CLICKED) {

        if((strcmp(LV_SYMBOL_LEFT, lv_label_get_text(label)) == 0))
        {
            
            tmp_node_ll = _lv_ll_get_prev(&photo_file_ll, g_node_ll);
            if(tmp_node_ll != NULL)
            {
                g_node_ll = _lv_ll_get_prev(&photo_file_ll, g_node_ll);
            }

            lv_snprintf(file_path_name, sizeof(file_path_name), "%s/%s", g_dir_path, g_node_ll->name);
            lv_img_set_src(g_img_photo_browser, file_path_name);
            LV_LOG_USER("Open %s", g_node_ll->name);
        }
        else if((strcmp(LV_SYMBOL_RIGHT, lv_label_get_text(label)) == 0))
        {
            tmp_node_ll = _lv_ll_get_next(&photo_file_ll, g_node_ll);
            if(tmp_node_ll != NULL)
            {
                g_node_ll = _lv_ll_get_next(&photo_file_ll, g_node_ll);
            }

            lv_snprintf(file_path_name, sizeof(file_path_name), "%s/%s", g_dir_path, g_node_ll->name);
            lv_img_set_src(g_img_photo_browser, file_path_name);
            LV_LOG_USER("Open %s", g_node_ll->name);
        }
    }
}



static bool is_end_with(const char * str1, const char * str2)
{
    if(str1 == NULL || str2 == NULL)
        return false;
    
    uint16_t len1 = strlen(str1);
    uint16_t len2 = strlen(str2);
    if((len1 < len2) || (len1 == 0 || len2 == 0))
        return false;
    
    while(len2 >= 1)
    {
        if(str2[len2 - 1] != str1[len1 - 1])
            return false;

        len2--;
        len1--;
    }

    return true;
}