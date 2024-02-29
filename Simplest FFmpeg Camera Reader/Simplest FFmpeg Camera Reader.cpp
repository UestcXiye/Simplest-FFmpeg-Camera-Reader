// Simplest FFmpeg Camera Reader.cpp : 定义控制台应用程序的入口点。
//

/**
* 最简单的基于 FFmpeg 的 AVDevice 例子（读取摄像头）
* Simplest FFmpeg Camera Reader
*
* 源程序：
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本程序实现了本地摄像头数据的获取解码和显示。
* 是基于 FFmpeg 的 libavdevice 类库最简单的例子。
* 通过该例子，可以学习 FFmpeg 中 libavdevice 类库的使用方法。
*
* 本程序在 Windows 下可以使用 2 种方式读取摄像头数据：
*  1.VFW: Video for Windows 屏幕捕捉设备。注意输入 URL 是设备的序号，从 0 至 9。
*  2.dshow: 使用 Directshow。注意作者机器上的摄像头设备名称是 "Integrated Camera"，
*    使用的时候需要改成自己电脑上摄像头设备的名称。
*
* 在 Linux 下可以使用 video4linux2 读取摄像头设备。
* 在 MacOS 下可以使用 avfoundation 读取摄像头设备。
*
* This software read data from Computer's Camera and play it.
* It's the simplest example about usage of FFmpeg's libavdevice Library.
* It's suiltable for the beginner of FFmpeg.
* This software support 2 methods to read camera in Microsoft Windows:
*  1.gdigrab: VfW (Video for Windows) capture input device.
*             The filename passed as input is the capture driver number,
*             ranging from 0 to 9.
*  2.dshow: Use Directshow. Camera's name in author's computer is
*             "Integrated Camera".
* It use video4linux2 to read Camera in Linux.
* It use avfoundation to read Camera in MacOS.
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

// 解决报错：'fopen': This function or variable may be unsafe.Consider using fopen_s instead.
#pragma warning(disable:4996)

// 解决报错：无法解析的外部符号 __imp__fprintf，该符号在函数 _ShowError 中被引用
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// 解决报错：无法解析的外部符号 __imp____iob_func，该符号在函数 _ShowError 中被引用
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
}

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "SDL/SDL.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <SDL/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

// Output YUV420P 
#define OUTPUT_YUV420P 0
// 1：Use Dshow; 0: Use VFW
#define USE_DSHOW 1

// Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit = 0;

int sfp_refresh_thread(void *opaque)
{
	thread_exit = 0;
	while (!thread_exit)
	{
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	thread_exit = 0;
	// Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

// Show Dshow Device
void show_dshow_device()
{
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat *iformat = av_find_input_format("dshow");
	printf("=============== Device Info ===============\n");
	avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
	printf("===========================================\n");
}

// Show Dshow Device Option
void show_dshow_device_option()
{
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_options", "true", 0);
	AVInputFormat *iformat = av_find_input_format("dshow");
	printf("\n============ Device Option Info ============\n");
	avformat_open_input(&pFormatCtx, "video=Integrated Camera", iformat, &options);
	printf("============================================\n");
}

// Show VFW Device
void show_vfw_device()
{
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVInputFormat *iformat = av_find_input_format("vfwcap");
	printf("\n============ VFW Device Info ============\n");
	avformat_open_input(&pFormatCtx, "list", iformat, NULL);
	printf("=========================================\n");
}

// Show AVFoundation Device
void show_avfoundation_device()
{
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat *iformat = av_find_input_format("avfoundation");
	printf("\n======= AVFoundation Device Info =======\n");
	avformat_open_input(&pFormatCtx, "", iformat, &options);
	printf("========================================\n");
}


int main(int argc, char* argv[])
{
	AVFormatContext	*pFormatCtx;
	int videoindex;
	int ret;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	// Open File
	//char filepath[] = "src01_480x272_22.h265";
	//avformat_open_input(&pFormatCtx, filepath, NULL, NULL);

	// Register Device
	avdevice_register_all();


	// Windows
#ifdef _WIN32
	// Show Dshow Device
	show_dshow_device();
	// Show Device Options
	show_dshow_device_option();
	// Show VFW Options
	show_vfw_device();

#if USE_DSHOW
	AVInputFormat *ifmt = av_find_input_format("dshow");
	// Set own video device's name
	if (avformat_open_input(&pFormatCtx, "video=Integrated Camera", ifmt, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	AVInputFormat *ifmt = av_find_input_format("vfwcap");
	if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif
#elif defined linux
	// Linux
	AVInputFormat *ifmt = av_find_input_format("video4linux2");
	if (avformat_open_input(&pFormatCtx, "/dev/video0", ifmt, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	show_avfoundation_device();
	// MacOS
	AVInputFormat *ifmt = av_find_input_format("avfoundation");
	// Avfoundation
	// [video]:[audio]
	if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif

	ret = avformat_find_stream_info(pFormatCtx, NULL);
	if (ret < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	videoindex = -1;
	for (size_t i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	ret = avcodec_open2(pCodecCtx, pCodec, NULL);
	if (ret < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	AVFrame	*pFrame, *pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	//unsigned char *out_buffer = (unsigned char *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P,
	//	pCodecCtx->width, pCodecCtx->height));
	//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P,
	//	pCodecCtx->width, pCodecCtx->height);


	// ------------------------ SDL 1.2 ------------------------
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s.\n", SDL_GetError());
		return -1;
	}
	int screen_w = 0, screen_h = 0;
	SDL_Surface *screen;
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	// 初始化屏幕（SDL 绘制的窗口）
	screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);

	if (!screen)
	{
		printf("SDL: could not set video mode - exiting:%s.\n", SDL_GetError());
		return -1;
	}
	SDL_Overlay *bmp;
	// Now we create a YUV overlay on that screen so we can input video to it
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, SDL_YV12_OVERLAY, screen);
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;
	// ------------------------ SDL End ------------------------

	int got_picture;

	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

#if OUTPUT_YUV420P 
	FILE *fp_yuv = fopen("output.yuv", "wb+");
#endif  

	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	// SDL 线程
	SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, NULL);
	// 设置窗口标题
	SDL_WM_SetCaption("Simplest FFmpeg Camera Reader", NULL);
	// Event Loop
	SDL_Event event;

	for (;;)
	{
		// Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT)
		{
			// Get an AVpacket
			if (av_read_frame(pFormatCtx, packet) >= 0)
			{
				if (packet->stream_index == videoindex)
				{
					ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
					if (ret < 0)
					{
						printf("Decode error.\n");
						return -1;
					}
					if (got_picture)
					{
						SDL_LockYUVOverlay(bmp);
						pFrameYUV->data[0] = bmp->pixels[0];
						pFrameYUV->data[1] = bmp->pixels[2];
						pFrameYUV->data[2] = bmp->pixels[1];
						pFrameYUV->linesize[0] = bmp->pitches[0];
						pFrameYUV->linesize[1] = bmp->pitches[2];
						pFrameYUV->linesize[2] = bmp->pitches[1];
						sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data,
							pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

#if OUTPUT_YUV420P  
						int y_size = pCodecCtx->width * pCodecCtx->height;
						fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv); // Y   
						fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv); // U  
						fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv); // V  
#endif  

						SDL_UnlockYUVOverlay(bmp);
						SDL_DisplayYUVOverlay(bmp, &rect);
					}
				}
				av_free_packet(packet);
			}
			else
			{
				// Exit Thread
				thread_exit = 1;
			}
		}
		else if (event.type == SDL_QUIT)
		{
			thread_exit = 1;
		}
		else if (event.type == SFM_BREAK_EVENT)
		{
			break;
		}
	}

	sws_freeContext(img_convert_ctx);

#if OUTPUT_YUV420P 
	fclose(fp_yuv);
#endif 

	SDL_Quit();

	// av_free(out_buffer);
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	system("pause");
	return 0;
}
