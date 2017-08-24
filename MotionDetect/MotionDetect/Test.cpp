#include <vector>
#include <iostream>

#include <stdio.h>
#include <time.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

using namespace std;
using namespace cv;
using namespace cv::ocl;

void ShowOpenCLInfo()
{
	vector<ocl::PlatformInfo> info;
	getPlatfomsInfo(info);
	PlatformInfo sdk = info.at(0);

	if (sdk.deviceNumber() < 1)
		return;

	cout << "******SDK*******" << endl;
	cout << "Name: " << sdk.name() << endl;
	cout << "Vendor: " << sdk.vendor() << endl;
	cout << "Version: " << sdk.version() << endl;
	cout << "Number of devices: " << sdk.deviceNumber() << endl;

	for (int i = 0; i < sdk.deviceNumber(); i++) {
		Device device;
		sdk.getDevice(device, i);
		cout << "\n*********************\n Device " << i + 1 << endl;

		cout << "Vendor ID: " << device.vendorID() << endl;
		cout << "Vendor name: " << device.vendorName() << endl;
		cout << "Name: " << device.name() << endl;
		cout << "Driver version: " << device.driverVersion() << endl;
		if (device.isAMD()) cout << "Is an AMD device" << endl;
		if (device.isIntel()) cout << "Is a Intel device" << endl;
		if (device.isNVidia()) cout << "Is a NVidia device" << endl;
		cout << "Global Memory size: " << device.globalMemSize() << endl;
		cout << "Memory cache size: " << device.globalMemCacheSize() << endl;
		cout << "Memory cache type: " << device.globalMemCacheType() << endl;
		cout << "Local Memory size: " << device.localMemSize() << endl;
		cout << "Local Memory type: " << device.localMemType() << endl;
		cout << "Max Clock frequency: " << device.maxClockFrequency() << endl;
		cout << "available:         " << device.available() << endl;
		cout << "imageSupport:      " << device.imageSupport() << endl;
		cout << "OpenCL_C_Version:  " << device.OpenCL_C_Version() << endl;
		cout << "Use OpenCL:        " << cv::ocl::useOpenCL() << endl;
	}
}

int main(int argc, char** argv)
{
	//����IplImageָ��
	IplImage* pFrame = NULL;     //pFrameΪ��Ƶ��ȡ��һ֡
	IplImage* pFrame1 = NULL;      //��һ֡
	IplImage* pFrame2 = NULL;//�ڶ�֡
	IplImage* pFrame3 = NULL;//����֡

	IplImage* pFrImg = NULL;     //pFrImgΪ��ǰ֡�ĻҶ�ͼ
	IplImage* pBkImg = NULL;     //pBkImgΪ��ǰ�����ĻҶ�ͼ
	IplImage* pBkImgTran = NULL;//pBkImgTranΪ��ǰ�����������ͼ��
	IplImage* pFrImgTran = NULL;//pFrImgTranΪ��ǰ�����������ͼ��

	CvMat* pFrameMat = NULL;     //pFrameMatΪ��ǰ�ҶȾ���
	CvMat* pFrMat = NULL;      //pFrMatΪ��ǰǰ��ͼ���󣬵�ǰ֡��ȥ����ͼ
	CvMat* bg1 = NULL;
	CvMat* bg2 = NULL;
	CvMat* bg3 = NULL;
	CvMat* pFrMatB = NULL;     //pFrMatBΪ��ֵ����0,1����ǰ��ͼ
	CvMat* pBkMat = NULL;
	CvMat* pZeroMat = NULL;               //���ڼ���bg1 - bg2 ��ֵ
	CvMat* pZeroMatB = NULL;//���ڼ��� pZeroMat��ֵ�������ж��ж��ٸ������ʱ����

	CvCapture* pCapture = NULL;

	int warningNum = 0;      //��⵽���������ֵĴ���
	int nFrmNum = 0;//֡����
	int status = 0;        //״̬��־λ

	ShowOpenCLInfo();
	
	if (argc > 2)
	{
		fprintf(stderr, "Usage: bkgrd [video_file_name]\n");
		return -1;
	}

	//��������
	cvNamedWindow("video", 1);
	cvNamedWindow("background", 1);//����
	cvNamedWindow("foreground", 1);//ǰ��
								   //ʹ������������
	cvMoveWindow("video", 30, 0);
	cvMoveWindow("background", 360, 0);
	cvMoveWindow("foreground", 690, 0);

	//������ͷ     ������ͷȡ����������ʹ���������Ƶ��������ģ������ͷ
	if (argc == 1)
		if (!(pCapture = cvCaptureFromCAM(-1)))
		{
			fprintf(stderr, "Can not open camera.\n");
			return -2;
		}

	//����Ƶ�ļ�
	if (argc == 2)
	{
		if (!(pCapture = cvCaptureFromFile(argv[1])))
		{
			fprintf(stderr, "Can not open video file %s\n", argv[1]);
			return -2;
		}
	}
		
	int nRet = cvSetCaptureProperty(pCapture, CV_CAP_PROP_SPEED, (double)2.0);
	
	//��ʼ��ʱ
	time_t start, end;
	time(&start);        //time() ���ش�1970��1��1��00��00��00��ʼ���������ڵ���������10Ϊ���֣���
	printf("%d\n", start);
	//��֡��ȡ��Ƶ
	while (pFrame = cvQueryFrame(pCapture))
	{
		nFrmNum++;
		//����ǵ�һ֡����Ҫ�����ڴ棬����ʼ��
		if (nFrmNum == 1)
		{
			pBkImg = cvCreateImage(cvSize(pFrame->width, pFrame->height), IPL_DEPTH_8U, 1);
			pFrImg = cvCreateImage(cvSize(pFrame->width, pFrame->height), IPL_DEPTH_8U, 1);
			pBkImgTran = cvCreateImage(cvSize(pFrame->width, pFrame->height), IPL_DEPTH_8U, 1);
			pFrImgTran = cvCreateImage(cvSize(pFrame->width, pFrame->height), IPL_DEPTH_8U, 1);

			pBkMat = cvCreateMat(pFrame->height, pFrame->width, CV_32FC1);
			pZeroMat = cvCreateMat(pFrame->height, pFrame->width, CV_32FC1);
			pFrMat = cvCreateMat(pFrame->height, pFrame->width, CV_32FC1);
			pFrMatB = cvCreateMat(pFrame->height, pFrame->width, CV_8UC1);
			pZeroMatB = cvCreateMat(pFrame->height, pFrame->width, CV_8UC1);
			pFrameMat = cvCreateMat(pFrame->height, pFrame->width, CV_32FC1);
			cvZero(pZeroMat);
			//ת���ɵ�ͨ��ͼ���ٴ���
			cvCvtColor(pFrame, pBkImg, CV_BGR2GRAY);
			//ת��Ϊ����
			cvConvert(pFrImg, pBkMat);
		}
		else /* ���ǵ�һ֡�ľ��������� */
		{
			//pFrImgΪ��ǰ֡�ĻҶ�ͼ
			cvCvtColor(pFrame, pFrImg, CV_BGR2GRAY);

			//pFrameMatΪ��ǰ�ҶȾ���
			cvConvert(pFrImg, pFrameMat);

			//pFrMatΪǰ��ͼ���󣬵�ǰ֡��ȥ����ͼ
			cvAbsDiff(pFrameMat, pBkMat, pFrMat);

			//pFrMatBΪ��ֵ����0,1����ǰ��ͼ
			cvThreshold(pFrMat, pFrMatB, 60, 1, CV_THRESH_BINARY);

			//��ͼ�����ת��Ϊͼ���ʽ��������ʾ
			cvConvert(pBkMat, pBkImgTran);
			cvConvert(pFrMat, pFrImgTran);

			//��ʾͼ��
			cvShowImage("video", pFrame);
			cvShowImage("background", pBkImgTran); //��ʾ����
			cvShowImage("foreground", pFrImgTran); //��ʾǰ��
			
			int nonZeroFrMatB = cvCountNonZero(pFrMatB);
			int maxNonZeroFrMatB = 0;

			//������ÿץȡһ֡��Ҫ���Ĺ������������Σ�ռ��
			if (nonZeroFrMatB > 10000 && status == 0) //��ʾ�ǵ�һ֡���������1W��������
			{/* ����Ҫ����ǰ֡�洢Ϊ��һ֡ */
				pFrame1 = cvCloneImage(pFrame);
				bg1 = cvCloneMat(pFrMat);
				status = 1;      //�����ɼ���2֡
			}
			else if (nonZeroFrMatB < 10000 && status == 1) // ��ʾ��һ֡���������1W�������㣬���ڶ�֡û��,�򱨾�
			{
				printf("NO.%d warning!!!!\n\n", warningNum++);
				status = 0;
			}
			else if (nonZeroFrMatB > 10000 && status == 1)// ��ʾ��һ֡�͵ڶ�֡�����ﶼ����1W��������
			{
				pFrame2 = cvCloneImage(pFrame);
				bg2 = cvCloneMat(pFrMat);

				cvAbsDiff(bg1, bg2, pZeroMat);
				cvThreshold(pZeroMat, pZeroMatB, 20, 1, CV_THRESH_BINARY);
				if (cvCountNonZero(pZeroMatB) > 3000) //��ʾ���ǲ������������Ļ�Ҫ����
				{
					printf("NO.%d warning!!!!\n\n", warningNum++);
					status = 0;
				}
				else
				{
					status = 2; //�����ɼ���3֡
				}
			}
			else if (nonZeroFrMatB < 10000 && status == 2)//��ʾ��һ֡�͵ڶ�֡�����ﶼ����1W��������,������֡û��
			{
				//����
				printf("NO.%d warning!!!!\n\n", warningNum++);
				status = 0;
			}
			else if (nonZeroFrMatB > 10000 && status == 2)//��ʾ����3֡�����ﶼ����1W��������
			{
				pFrame3 = cvCloneImage(pFrame);
				bg3 = cvCloneMat(pFrMat);

				cvAbsDiff(bg2, bg3, pZeroMat);
				cvThreshold(pZeroMat, pZeroMatB, 20, 1, CV_THRESH_BINARY);
				if (cvCountNonZero(pZeroMatB) > 3000) //��ʾ���ǲ������������Ļ�Ҫ����
				{
					printf("NO.%d warning!!!!\n\n", warningNum++);
				}
				else //��ʾbg2,bg3����
				{
					cvReleaseMat(&pBkMat);
					pBkMat = cvCloneMat(pFrameMat); //���±���
				}
				status = 0;                //������һ�βɼ�����
			}

			//����а����¼���������ѭ��
			//�˵ȴ�ҲΪcvShowImage�����ṩʱ�������ʾ
			//�ȴ�ʱ����Ը���CPU�ٶȵ���
			if (cvWaitKey(2) >= 0)
				break;
		}/* The End of the else */
	}/* The End of th while */

	 //���ٴ���
	cvDestroyWindow("video");
	cvDestroyWindow("background");
	cvDestroyWindow("foreground");

	//�ͷ�ͼ��;���
	cvReleaseImage(&pFrImg);
	cvReleaseImage(&pBkImg);

	cvReleaseMat(&pFrameMat);
	cvReleaseMat(&pFrMat);
	cvReleaseMat(&pBkMat);

	cvReleaseCapture(&pCapture);

	printf("Press enter to exit...\n");
	getchar();

	return 0;
}
