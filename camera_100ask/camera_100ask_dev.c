#include "camera_100ask_dev.h"
#include "convert_to_bmp_file.h"

#include <time.h>
#include <string.h>
#include <pthread.h>

static T_VideoDevice tVideoDevice;
static PT_VideoConvert ptVideoConvert;
static int iPixelFormatOfVideo;
static int iPixelFormatOfDisp;

static PT_VideoBuf ptVideoBufCur;
static T_VideoBuf tVideoBuf;
static T_VideoBuf tConvertBuf;
static T_VideoBuf tZoomBuf;
static T_VideoBuf tFrameBuf;

static int iLcdWidth;
static int iLcdHeigt;
static int iLcdBpp;

static int g_brightness_value = 0;

static camera_100ask_opt_t g_camera_opt = CAMERA_100ASK_OPT_NONE;

pthread_mutex_t g_camera_100ask_mutex;

static void *thread_camera_work(void *args);
static void camera_set_brightness(int value);

int camera_100ask_dev_init(char * dev)
{
    int iError;

    pthread_mutex_init(&g_camera_100ask_mutex, NULL);

    /* 注册显示设备 */
	DisplayInit();
	/* 可能可支持多个显示设备: 选择和初始化指定的显示设备 */
	SelectAndInitDefaultDispDev("fb");
    GetDispResolution(&iLcdWidth, &iLcdHeigt, &iLcdBpp);
    GetVideoBufForDisplay(&tFrameBuf);
    iPixelFormatOfDisp = tFrameBuf.iPixelFormat;
    
    VideoInit();

    iError = VideoDeviceInit(dev, &tVideoDevice);
    if (iError)
    {
        DBG_PRINTF("VideoDeviceInit for %s error!\n", dev);
        return -1;
    }
    iPixelFormatOfVideo = tVideoDevice.ptOPr->GetFormat(&tVideoDevice);

    VideoConvertInit();
    ptVideoConvert = GetVideoConvertForFormats(iPixelFormatOfVideo, iPixelFormatOfDisp);
    if (NULL == ptVideoConvert)
    {
        DBG_PRINTF("can not support this format convert\n");
        return -1;
    }

    /* 启动摄像头设备 */
    iError = tVideoDevice.ptOPr->StartDevice(&tVideoDevice);
    if (iError)
    {
        DBG_PRINTF("StartDevice for %s error!\n", dev);
        return -1;
    }

    memset(&tVideoBuf, 0, sizeof(tVideoBuf));
    memset(&tConvertBuf, 0, sizeof(tConvertBuf));
    tConvertBuf.iPixelFormat     = iPixelFormatOfDisp;
    tConvertBuf.tPixelDatas.iBpp = iLcdBpp;
    
    memset(&tZoomBuf, 0, sizeof(tZoomBuf));

    /* 创建线程 */
    pthread_t thread;
    pthread_create(&thread, NULL, thread_camera_work, NULL);

    return 0;
}

void camera_100ask_dev_set_opt(camera_100ask_opt_t opt)
{
    g_camera_opt = opt;
}

void camera_100ask_dev_set_brightness(int value)
{
    g_brightness_value = value;
}

PT_VideoBuf camera_100ask_dev_get_video_buf_cur(void)
{
    return ptVideoBufCur;
}

static void camera_set_brightness(int value)
{
    PT_VideoDevice ptVideoDevice = &tVideoDevice;

    struct v4l2_queryctrl   qctrl;
    memset(&qctrl, 0, sizeof(qctrl));
    qctrl.id = V4L2_CID_BRIGHTNESS; // V4L2_CID_BASE+0;
    if (0 != ioctl(ptVideoDevice->iFd, VIDIOC_QUERYCTRL, &qctrl))
    {
        printf("can not query brightness\n");
        return;
    }

    //printf("current value=%d, brightness min = %d, max = %d\n", value, qctrl.minimum, qctrl.maximum);
        
    struct v4l2_control ctl;
    ctl.id = V4L2_CID_BRIGHTNESS; // V4L2_CID_BASE+0;
    ioctl(ptVideoDevice->iFd, VIDIOC_G_CTRL, &ctl);

    ctl.value = value;

    if (ctl.value > qctrl.maximum)
        ctl.value = qctrl.maximum;
    if (ctl.value < qctrl.minimum)
        ctl.value = qctrl.minimum;
    
    ioctl(ptVideoDevice->iFd, VIDIOC_S_CTRL, &ctl);

}

static void *thread_camera_work(void *args)
{
    int iError;
    float k;
    int iTopLeftX;
    int iTopLeftY;

    time_t timep;
    struct tm *p;
    char time_buffer [64];

    while(1)
    {
        pthread_mutex_lock(&g_camera_100ask_mutex);
        /* 读入摄像头数据 */
        int iError;
        iError = tVideoDevice.ptOPr->GetFrame(&tVideoDevice, &tVideoBuf);
        if (iError)
        {
            DBG_PRINTF("GetFrame for error!\n");
            //return;
        }
        ptVideoBufCur = &tVideoBuf;

        if (iPixelFormatOfVideo != iPixelFormatOfDisp)
        {
            /* 转换为RGB */
            iError = ptVideoConvert->Convert(&tVideoBuf, &tConvertBuf);
            //DBG_PRINTF("Convert %s, ret = %d\n", ptVideoConvert->name, iError);
            if (iError)
            {
                DBG_PRINTF("Convert for error!\n");
                //return;
            }            
            ptVideoBufCur = &tConvertBuf;
        }

        /* 如果图像分辨率大于LCD, 缩放 */
        if (ptVideoBufCur->tPixelDatas.iWidth > iLcdWidth || ptVideoBufCur->tPixelDatas.iHeight > iLcdHeigt)
        {
            /* 确定缩放后的分辨率 */
            /* 把图片按比例缩放到VideoMem上, 居中显示
             * 1. 先算出缩放后的大小
             */
            k = (float)ptVideoBufCur->tPixelDatas.iHeight / ptVideoBufCur->tPixelDatas.iWidth;
            tZoomBuf.tPixelDatas.iWidth  = iLcdWidth;
            tZoomBuf.tPixelDatas.iHeight = iLcdWidth * k;
            if ( tZoomBuf.tPixelDatas.iHeight > iLcdHeigt)
            {
                tZoomBuf.tPixelDatas.iWidth  = iLcdHeigt / k;
                tZoomBuf.tPixelDatas.iHeight = iLcdHeigt;
            }
            tZoomBuf.tPixelDatas.iBpp        = iLcdBpp;
            tZoomBuf.tPixelDatas.iLineBytes  = tZoomBuf.tPixelDatas.iWidth * tZoomBuf.tPixelDatas.iBpp / 8;
            tZoomBuf.tPixelDatas.iTotalBytes = tZoomBuf.tPixelDatas.iLineBytes * tZoomBuf.tPixelDatas.iHeight;

            if (!tZoomBuf.tPixelDatas.aucPixelDatas)
            {
                tZoomBuf.tPixelDatas.aucPixelDatas = malloc(tZoomBuf.tPixelDatas.iTotalBytes);
            }
            
            PicZoom(&ptVideoBufCur->tPixelDatas, &tZoomBuf.tPixelDatas);
            ptVideoBufCur = &tZoomBuf;
        }

        /* 合并进framebuffer */
        /* 接着算出居中显示时左上角坐标 */
        //iTopLeftX = (iLcdWidth - ptVideoBufCur->tPixelDatas.iWidth) / 2;
        //iTopLeftY = (iLcdHeigt - ptVideoBufCur->tPixelDatas.iHeight) / 2;

        //PicMerge(iTopLeftX, iTopLeftY, &ptVideoBufCur->tPixelDatas, &tFrameBuf.tPixelDatas);

        //FlushPixelDatasToDev(&tFrameBuf.tPixelDatas);

        iError = tVideoDevice.ptOPr->PutFrame(&tVideoDevice, &tVideoBuf);
        if (iError)
        {
            DBG_PRINTF("PutFrame for error!\n");
        }                    

        pthread_mutex_unlock(&g_camera_100ask_mutex);

        switch (g_camera_opt)
        {
            case CAMERA_100ASK_OPT_UPDATE_BRIGHTNESS:
                camera_set_brightness(g_brightness_value);
                g_camera_opt = CAMERA_100ASK_OPT_NONE;
                break;
            case CAMERA_100ASK_OPT_TAKE_PHOTOS:
                time (&timep);
                p=gmtime(&timep);
                strftime (time_buffer, sizeof(time_buffer),"100ask-picture-%Y%m%d-%H%M%S.bmp",p);
                printf("photos name: %s\n", time_buffer);
                CvtRgb2BMPFileFrmFrameBuffer(ptVideoBufCur->tPixelDatas.aucPixelDatas, ptVideoBufCur->tPixelDatas.iWidth, ptVideoBufCur->tPixelDatas.iHeight, ptVideoBufCur->tPixelDatas.iBpp, time_buffer);
                g_camera_opt = CAMERA_100ASK_OPT_NONE;
                break;
            case CAMERA_100ASK_OPT_TAKE_VIDEO:
                g_camera_opt = CAMERA_100ASK_OPT_NONE;
                break;
            case CAMERA_100ASK_OPT_NONE:
            default:
                break;
        }
    }

    return NULL;
}

