#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <disp_manager.h>
#include <video_manager.h>
#include <convert_manager.h>
#include <render.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>


/* video2lcd </dev/video0,1,...> */
int main(int argc, char **argv)
{	
	int iError;
    T_VideoDevice tVideoDevice;
    PT_VideoConvert ptVideoConvert;
    int iPixelFormatOfVideo;
    int iPixelFormatOfDisp;

    PT_VideoBuf ptVideoBufCur;
    T_VideoBuf tVideoBuf;
    T_VideoBuf tConvertBuf;
    T_VideoBuf tZoomBuf;
    T_VideoBuf tFrameBuf;
    
    int iLcdWidth;
    int iLcdHeigt;
    int iLcdBpp;

    int iTopLeftX;
    int iTopLeftY;

    float k;
    
    if (argc != 2)
    {
        printf("Usage:\n");
        printf("%s </dev/video0,1,...>\n", argv[0]);
        return -1;
    }
    
    

    /* һϵ�еĳ�ʼ�� */
	/* ע����ʾ�豸 */
	DisplayInit();
	/* ���ܿ�֧�ֶ����ʾ�豸: ѡ��ͳ�ʼ��ָ������ʾ�豸 */
	SelectAndInitDefaultDispDev("fb");
    GetDispResolution(&iLcdWidth, &iLcdHeigt, &iLcdBpp);
    GetVideoBufForDisplay(&tFrameBuf);
    iPixelFormatOfDisp = tFrameBuf.iPixelFormat;

    VideoInit();

    iError = VideoDeviceInit(argv[1], &tVideoDevice);
    if (iError)
    {
        DBG_PRINTF("VideoDeviceInit for %s error!\n", argv[1]);
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


    /* ��������ͷ�豸 */
    iError = tVideoDevice.ptOPr->StartDevice(&tVideoDevice);
    if (iError)
    {
        DBG_PRINTF("StartDevice for %s error!\n", argv[1]);
        return -1;
    }

    memset(&tVideoBuf, 0, sizeof(tVideoBuf));
    memset(&tConvertBuf, 0, sizeof(tConvertBuf));
    tConvertBuf.iPixelFormat     = iPixelFormatOfDisp;
    tConvertBuf.tPixelDatas.iBpp = iLcdBpp;
    
    
    memset(&tZoomBuf, 0, sizeof(tZoomBuf));
    

    while (1)
    {
        /* ��������ͷ���� */
        iError = tVideoDevice.ptOPr->GetFrame(&tVideoDevice, &tVideoBuf);
        if (iError)
        {
            DBG_PRINTF("GetFrame for %s error!\n", argv[1]);
            return -1;
        }
        ptVideoBufCur = &tVideoBuf;

        if (iPixelFormatOfVideo != iPixelFormatOfDisp)
        {
            /* ת��ΪRGB */
            iError = ptVideoConvert->Convert(&tVideoBuf, &tConvertBuf);
            DBG_PRINTF("Convert %s, ret = %d\n", ptVideoConvert->name, iError);
            if (iError)
            {
                DBG_PRINTF("Convert for %s error!\n", argv[1]);
                return -1;
            }            
            ptVideoBufCur = &tConvertBuf;
        }
        

        /* ���ͼ��ֱ��ʴ���LCD, ���� */
        if (ptVideoBufCur->tPixelDatas.iWidth > iLcdWidth || ptVideoBufCur->tPixelDatas.iHeight > iLcdHeigt)
        {
            /* ȷ�����ź�ķֱ��� */
            /* ��ͼƬ���������ŵ�VideoMem��, ������ʾ
             * 1. ��������ź�Ĵ�С
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

        /* �ϲ���framebuffer */
        /* �������������ʾʱ���Ͻ����� */
        iTopLeftX = (iLcdWidth - ptVideoBufCur->tPixelDatas.iWidth) / 2;
        iTopLeftY = (iLcdHeigt - ptVideoBufCur->tPixelDatas.iHeight) / 2;

        PicMerge(iTopLeftX, iTopLeftY, &ptVideoBufCur->tPixelDatas, &tFrameBuf.tPixelDatas);

        FlushPixelDatasToDev(&tFrameBuf.tPixelDatas);

        iError = tVideoDevice.ptOPr->PutFrame(&tVideoDevice, &tVideoBuf);
        if (iError)
        {
            DBG_PRINTF("PutFrame for %s error!\n", argv[1]);
            return -1;
        }                    

        /* ��framebuffer������ˢ��LCD��, ��ʾ */
    }
		
	return 0;
}

