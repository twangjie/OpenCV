#pragma once

#include <opencv2/core/core.hpp>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavfilter/avfilter.h"
#include <libavutil/imgutils.h>
};

class ffmpegDecode
{
public:
	ffmpegDecode(char * file = NULL);
	~ffmpegDecode();

	cv::Mat getDecodedFrame();
	cv::Mat getLastFrame();
	int readOneFrame();
	int getFrameInterval();

private:
	AVFrame    *pAvFrame;
	AVFormatContext    *pFormatCtx;
	AVCodecContext    *pCodecCtx;
	AVCodec            *pCodec;

	int    i;
	int videoindex;

	char *filepath;
	int ret, got_picture;
	SwsContext *img_convert_ctx;
	int y_size;
	AVPacket *packet;

	cv::Mat *m_pCvMat;

	void init();
	void openDecode();
	void prepare();
	void get(AVCodecContext *pCodecCtx, SwsContext *img_convert_ctx, AVFrame    *pFrame);
};

