#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
#include <windows.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

#include <exception>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>

#include "ffmpegDecode.h"

using namespace std;

int test(int argc, char* argv[]);
int test2(int argc, char* argv[]);
int test3(int argc, char *argv[]);

static void fill_iplimage_from_frame(IplImage *img, const AVFrame *frame, enum AVPixelFormat pixfmt)
{
	IplImage *tmpimg;
	int depth, channels_nb;

	if (pixfmt == AV_PIX_FMT_GRAY8) { depth = IPL_DEPTH_8U;  channels_nb = 1; }
	else if (pixfmt == AV_PIX_FMT_BGRA) { depth = IPL_DEPTH_8U;  channels_nb = 4; }
	else if (pixfmt == AV_PIX_FMT_BGR24) { depth = IPL_DEPTH_8U;  channels_nb = 3; }
	else return;

	tmpimg = cvCreateImageHeader(cvSize(frame->width, frame->height), depth, channels_nb);
	*img = *tmpimg;
	img->imageData = img->imageDataOrigin = (char *)frame->data[0];
	img->dataOrder = IPL_DATA_ORDER_PIXEL;
	img->origin = IPL_ORIGIN_TL;
	img->widthStep = frame->linesize[0];
}

int main(int argc, char* argv[])
{
	//test(argc, argv);
	//test2(argc, argv);
	test3(argc, argv);

	getchar();

	return 0;
}

int test2(int argc, char* argv[])
{
	char *filepath = "bigbuckbunny_480x272.h265";
	if (argc >= 2)
	{
		filepath = argv[1];
	}

	char *pWindowName = "video";

	cvNamedWindow(pWindowName, CV_WINDOW_AUTOSIZE);

	ffmpegDecode *pFfdecode = new ffmpegDecode(filepath);
	cv::Mat mat;

	while (pFfdecode->readOneFrame() == 0)
	{
		mat = pFfdecode->getDecodedFrame();

		try
		{
			IplImage iplimage(mat);
			//cvShowImage(pWindowName, &iplimage);
			imshow(pWindowName, mat);
		}
		catch (std::exception &e)
		{
			printf(e.what());
		}
	}

	getchar();
	cvDestroyWindow(pWindowName);

	return 0;
}

int test(int argc, char* argv[])
{
	char *filepath = "bigbuckbunny_480x272.h265";
	char *dst_size = "800x600";
	int dst_w = 800, dst_h = 600;

	if (argc == 2)
	{
		filepath = argv[1];
	}

	switch (argc)
	{
	case 3:
		dst_size = argv[2];
	case 2:
		filepath = argv[1];
		break;
	default:
		return 0;
	}

	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame;
	//uint8_t *out_bufferRGB;
	AVPacket *packet;
	int ret = 0;
	int got_picture = 0;
	struct SwsContext *img_convert_ctx;

	IplImage* ipl = NULL;

	cvNamedWindow("video", CV_WINDOW_AUTOSIZE);
	cvMoveWindow("video", 30, 0);

	av_register_all();
	avformat_network_init();

	if (av_parse_video_size(&dst_w, &dst_h, dst_size) < 0) {
		fprintf(stderr,
			"Invalid size '%s', must be in the form WxH or a valid size abbreviation\n",
			dst_size);
		exit(1);
	}

	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.\n");
		return -1;
	}

	// opencv��RGB����˳��ΪBGR������ѡ��AV_PIX_FMT_BGR24�ı��뷽ʽ��
	AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P;

	pFrame = av_frame_alloc();

	uint8_t *video_dst_data[4] = {};
	int      video_dst_linesize[4] = {};
	int video_dst_bufsize;

	//int size = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
	//int size = av_image_get_buffer_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height, 8);

//  	out_bufferRGB = (uint8_t *)av_malloc(size);
//  	avpicture_fill((AVPicture *)pFrameRGB, out_bufferRGB, pixfmt, pCodecCtx->width, pCodecCtx->height);

	video_dst_bufsize = av_image_alloc(video_dst_data, video_dst_linesize,
		dst_w, dst_h, dst_pix_fmt, 1);

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
		pCodecCtx->pix_fmt, dst_w, dst_h,
		dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	//Output Info-----------------------------
	printf("---------------- File Information ---------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	//Event Loop
	int nCount = 0;
	for (;;)
	{
		if (av_read_frame(pFormatCtx, packet) >= 0)
		{
			if (packet->stream_index == videoindex)
			{
				nCount++;

				ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
				if (ret < 0) {
					printf("Decode Error.\n");
					break;
				}
				if (got_picture)
				{
					if (img_convert_ctx == NULL)
					{
						// opencv��RGB����˳��ΪBGR������ѡ��AV_PIX_FMT_BGR24�ı��뷽ʽ��
						img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
							dst_w, dst_h, dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
					}

					//sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
					int nRet = sws_scale(img_convert_ctx, (const uint8_t * const*)pFrame->data,
						pFrame->linesize, 0, pCodecCtx->height, video_dst_data, video_dst_linesize);
					
					ipl = cvCreateImage(cvSize(pCodecCtx->width, pCodecCtx->height), IPL_DEPTH_8U, 3);

					//	fill_iplimage_from_frame(ipl, pFrame, AV_PIX_FMT_BGR24);

					memcpy(ipl->imageData, video_dst_data[0], video_dst_bufsize);
					ipl->widthStep = pCodecCtx->width * 3;
					ipl->origin = 0;

					try
					{
						cvShowImage("video", ipl);

						cvReleaseImageHeader(&ipl);
						cvReleaseImage(&ipl);
					}
					catch (std::exception &e)
					{
						printf(e.what());
					}

#ifdef USE_SDL
					//SDL---------------------------
					SDL_UpdateTexture(sdlTexture, NULL, pFrameRGB->data[0], pFrameRGB->linesize[0]);
					SDL_RenderClear(sdlRenderer);
					//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
					SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
					SDL_RenderPresent(sdlRenderer);
					//SDL End-----------------------
#endif

					
					Sleep(40);

					av_frame_unref(pFrame);
				}
			}

			av_packet_unref(packet);
		}
		else {
			break;
		}
	}

	sws_freeContext(img_convert_ctx);

	//av_frame_free(&pFrameRGB);
	av_frame_free(&pFrame);

	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}

static void cvDisplayRGB(uchar *pFrameData, int width, int height, int time)
{
	static bool bWindowNamed = false;

	if (time <= 0) time = 1;

	int		nChannels;
	int		stepWidth;
	uchar  *	pData;
	cv::Mat frameImage(cv::Size(width, height), CV_8UC3, cv::Scalar(0));
	stepWidth = frameImage.step;
	nChannels = frameImage.channels();
 	pData = frameImage.data;
// 	
// 	memcpy(pData, pFrameData, height * width * nChannels);

	frameImage.data = pFrameData;

	if (!bWindowNamed)
	{
		try
		{
			cv::namedWindow("Video", CV_WINDOW_AUTOSIZE);
			bWindowNamed = true;
		}
		catch (exception &e)
		{
			printf(e.what());
		}
	}

	cv::imshow("Video", frameImage);
	cv::waitKey(time);
	//cv::waitKey(40);

	frameImage.data = pData;
}

int test3(int argc, char *argv[])
{
	AVFormatContext *pFormatCtx = NULL;
	int             i, videoStream;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame;
	//AVFrame         *pFrameDst;
	AVPacket        packet;
	int             bGotDecodedFrame;
	int             numBytes;
	//uint8_t         *buffer;

	char			*pRGBDumpFilePath = "out-test.dat";
	FILE			*pRgbDumpFile = NULL;
	char			*output_size = "640x480";

	int dstWidth = 640;
	int dstHeight = 480;
	AVPixelFormat	dstPixFmt = AV_PIX_FMT_BGR24;

	char *pInputFile = "test.h264";
	switch (argc)
	{
	case 4:
		output_size = argv[3];
	case 3:
		pRGBDumpFilePath = argv[2];
	case 2:
		pInputFile = argv[1];
		break;
	default:
		break;
	}

	if (pRGBDumpFilePath)
	{
		pRgbDumpFile = fopen(pRGBDumpFilePath, "wb");
		if (!pRgbDumpFile) {
			fprintf(stderr, "Could not open destination file %s\n", "out.dat");
			exit(1);
		}
	}

	if (output_size && av_parse_video_size(&dstWidth, &dstHeight, output_size) < 0) {
		fprintf(stderr,
			"Invalid size '%s', must be in the form WxH or a valid size abbreviation\n",
			output_size);
		exit(1);
	}


	/*ע�������е��ļ���ʽ�ͱ�������Ŀ�*/
	av_register_all();

	// ����Ƶ�ļ�
	/*���������ȡ�ļ���ͷ�����Ұ���Ϣ���浽���Ǹ���AVFormatContext�ṹ���С�
	*�ڶ������� Ҫ�򿪵��ļ�·��
	*/
	if (avformat_open_input(&pFormatCtx, pInputFile, NULL, NULL) != 0) {
		return -1; // Couldn't open file
	}

	// ��ȡ���ݰ���ȡ��ý���ļ�����Ϣ
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		return -1; // Couldn't find stream information
	}

	//��ӡ����������ʽ����ϸ��Ϣ,��ʱ�䡢������,Ϫ��,����,����,Ԫ����,��������,���������ʱ�䡣
	av_dump_format(pFormatCtx, 0, pInputFile, false);

	//���ҵ�һ����Ƶ��
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}

	if (videoStream == -1) {//δ������Ƶ���˳�
		return -1;
	}

	//�����Ƶ���������������
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	// �ҵ���Ƶ������
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {//δ������Ƶ������
		return -1;
	}

	// ����Ƶ������
	if (avcodec_open2(pCodecCtx, pCodec, 0) < 0) {
		return -1; //��ʧ���˳�
	}

	/*��Щ�˿��ܻ�Ӿɵ�ָ���мǵ�������������Щ�����������֣�
	*���CODEC_FLAG_TRUNCATED�� pCodecCtx->flags�����һ��hack���ֲڵ�����֡�ʡ�
	*�����������Ѿ����ڴ�����ffplay.c�С���ˣ��ұ���������ǲ��ٱ� Ҫ�������Ƴ�
	*����Щ�������һ����Ҫָ���Ĳ�ͬ�㣺pCodecCtx->time_base�����Ѿ�������֡��
	*����Ϣ��time_base��һ ���ṹ�壬��������һ�����Ӻͷ�ĸ (AVRational)������ʹ
	*�÷����ķ�ʽ����ʾ֡������Ϊ�ܶ�������ʹ�÷�������֡�ʣ�����NTSCʹ��29.97fps����
	*
	*if(pCodecCtx->time_base.num > 1000 && pCodecCtx->time_base.den == 1){
	*	pCodecCtx->time_base.den = 1000;
	*}
	*/

	// ���䱣����Ƶ֡�Ŀռ�
	pFrame = av_frame_alloc();

	uint8_t *video_dst_data[4] = {};
	int     video_dst_linesize[4] = {};
	int		video_dst_bufsize = 0;

	video_dst_bufsize = av_image_alloc(video_dst_data, video_dst_linesize,
		dstWidth, dstHeight, dstPixFmt, 1);

	/*��ȡ����֡
	*ͨ����ȡ������ȡ������Ƶ����Ȼ����������֡����ú�ת����ʽ���ұ��档
	*/

	i = 0;
	long prepts = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		
		/*�ж϶�ȡ���Ƿ�Ϊ��Ҫ����Ƶ֡����*/
		if (packet.stream_index == videoStream) 
		{
			// ������Ƶ֡
			avcodec_decode_video2(pCodecCtx, pFrame, &bGotDecodedFrame, &packet);

			if (bGotDecodedFrame)
			{
				pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);
				AVRational frame_rate = av_guess_frame_rate(pFormatCtx, pFormatCtx->streams[videoStream], pFrame);
				AVRational frame_rate_tmp = { frame_rate.den, frame_rate.num };
				double duration = (frame_rate.num && frame_rate.den ? av_q2d(frame_rate_tmp) : 0);

				static struct SwsContext *img_convert_ctx;

				if (img_convert_ctx == NULL) {

					//����ͷ���һ��SwsContext������Ҫִ������/ת������ʹ��sws_scale()��
					img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
						dstWidth, dstHeight, dstPixFmt, SWS_BICUBIC, NULL, NULL, NULL);
					if (img_convert_ctx == NULL) {
						fprintf(stderr, "Cannot initialize the conversion context!\n");
						exit(1);
					}
				}

				////ת��ͼ���ʽ������ѹ������YUV420P��ͼ��ת��ΪBRG24��ͼ��

				sws_scale(img_convert_ctx, (const uint8_t * const*)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height, video_dst_data, video_dst_linesize);

				if (i++ <= 100)
				{
					if (pRgbDumpFile) fwrite(video_dst_data[0], 1, video_dst_bufsize, pRgbDumpFile);
				}

				cvDisplayRGB(video_dst_data[0], dstWidth, dstHeight, (int)(duration * 1000));
				prepts = pFrame->pts;
			}
		}

		//�ͷſռ�
		av_packet_unref(&packet);
	}

	if (pRgbDumpFile)
	{
		fclose(pRgbDumpFile);
		fprintf(stderr, "Scaling succeeded. Play the output file with the command:\n"
			"ffplay -f rawvideo -pixel_format %s -video_size %dx%d %s\n",
			av_get_pix_fmt_name(dstPixFmt), dstWidth, dstHeight, pRGBDumpFilePath);
	}


	//�ͷſռ�
	//av_free(buffer);
	//av_free(pFrameDst);
	av_freep(video_dst_data);

	// �ͷ� YUV frame
	av_free(pFrame);

	//�رս�����
	avcodec_close(pCodecCtx);

	//�ر���Ƶ�ļ�
	avformat_close_input(&pFormatCtx);

	system("Pause");

	return 0;
}