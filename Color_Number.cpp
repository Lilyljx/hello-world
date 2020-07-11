#include <iostream>
#include <opencv2/opencv.hpp>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


using namespace std;
using namespace cv;

Mat frame, roi, temp;

void numRecognizer();
int main()
{
	VideoCapture cap(0);


//数字识别
	namedWindow("Control", CV_WINDOW_AUTOSIZE);//create a window called "Control"
	int iLowH = 100;//Blue100,100-124;red0-10,156-180
	int iHighH = 140;//140

	int iLowS = 90;//Blue90,43-255;red43-255
	int iHighS = 255;//255

	int iLowV = 90;//Blue90,46-255;46-255
	int iHighV = 255;//255

	//Create trackbars in "Control" window
	cvCreateTrackbar("LowH", "Control", &iLowH, 179);//Hue(0-179)
	cvCreateTrackbar("HighH", "Control", &iHighH, 179);

	cvCreateTrackbar("LowS", "Control", &iLowS, 255);//Saturation(0-255)
	cvCreateTrackbar("HighS", "Control", &iHighS, 255);

	cvCreateTrackbar("LowV", "Control", &iLowV, 255);//Value(0-255)
	cvCreateTrackbar("HighV", "Control", &iHighV, 255);

	while (true)
	{
		Mat imgOriginal;

		bool bSuccess = cap.read(imgOriginal); // read a new frame from video
		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		Mat imgHSV;
		vector<Mat> hsvSplit;
		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
													  //因为我们读取的是彩色图，直方图均衡化需要在HSV空间做
		split(imgHSV, hsvSplit);
		equalizeHist(hsvSplit[2], hsvSplit[2]);
		merge(hsvSplit, imgHSV);
		
		Mat imgThresholded;
		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image
		//开操作 (去除一些噪点)
		Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));
		morphologyEx(imgThresholded, imgThresholded, MORPH_OPEN, element);

		//闭操作 (连接一些连通域)
		morphologyEx(imgThresholded, imgThresholded, MORPH_CLOSE, element);

		imshow("Thresholded Image", imgThresholded); //送花ow the thresholded image
		imshow("Original", imgOriginal); //送花ow the original image

		char key = (char)waitKey(300);
		if (key == (int)'1')
		break;
	}

    temp = imread("num.jpg", 0);  // 模板，原图0~9

	while (cap.isOpened())
	{
		cap >> frame;
		roi = frame(Rect(20, 80, 600, 100)).clone();//提取图像区域
		blur(frame, frame, Size(15, 15));//滤波
		roi.copyTo(frame(Rect(20, 80, 600, 100))); //将图像区域复制到框架
		rectangle(frame, Point(20, 80), Point(620, 180), Scalar(0, 200, 0), 2);
		imshow("数字识别", frame);
		int key = waitKey(20);
		if (frame.empty() || key == 27)
			break;//Esc退出
		if (key == (int)'0')
			numRecognizer();//0键识别
	}
	return 0;
}
void numRecognizer()
{	
	Mat src;	
	cvtColor(roi, src, CV_BGR2GRAY);//灰度处理
	threshold(src, src, 100, 255, THRESH_BINARY_INV);//二值化
	Mat element = getStructuringElement(MORPH_RECT, Size(1, 1));//返回指定形状-矩形 和尺寸的结构元素-中心点位置
	morphologyEx(src, src, MORPH_OPEN, element);//膨胀腐蚀组合	
	imwrite("num.png", src);
	int row_begin, row_end, col_begin, col_end;//去掉带有数字的图片上下左右的黑边/白边
											   //这四个数用于表示去掉黑边，数字区域在src中的行列坐标		
	for (int i = 0; i < src.rows - 1; i++)	
	{		
		Mat row = src.row(i);		
		int sum = 0;		
		for (int j = 0; j < src.cols; j++)		
		{			
			sum += row.data[j];		
		}		
		if (sum)		
		{			
			row_begin = i;  // 得到 row_begin			
			break;		
		}	
	} 	
	for (int i = src.rows; i > 0; i--)	
	{		
		Mat row = src.row(i - 1);		
		int sum = 0;		
		for (int j = 0; j < src.cols; j++)		
		{			
			sum += row.data[j];		
		}		
		if (sum)		
		{			
			row_end = i;			
			break;		
		}	
	} 	
	for (int i = 0; i < src.cols - 1; i++)	
	{		
		Mat col = src.col(i).clone();		
		int sum = 0;		
		for (int j = 0; j < src.rows; j++)		
		{			
			sum += col.data[j];		
		}		
		if (sum)		
		{			
			col_begin = i;			
			break;		
		}	
	} 	
	for (int i = src.cols; i > 0; i--)	
	{		
		Mat col = src.col(i - 1).clone();		
		int sum = 0;		
		for (int j = 0; j < src.rows; j++)		
		{			
			sum += col.data[j];		
		}		
		if (sum)		
		{			
			col_end = i;			
			break;		
		}	
	} 	

//开始提取每一个数字，进行匹配识别 	
	Mat dst = src(Range(row_begin, row_end), Range(col_begin, col_end));//去掉黑边的图片	
	resize(temp, temp, Size(cvRound(7.1 * dst.rows), dst.rows));//改变temp的尺寸，与dst等高 	
	int col_count_begin = 0;//从左向右分析图片中的数字，这两个变量用于分析当前分析到图片的哪一列	
	int col_count_end = 0;	
	bool end = false; 	
	while (!end)//开始循环
	{		
		for (int i = col_count_end + 1; i < dst.cols - 1; i++)		
		{			
			Mat col = dst.col(i).clone();			
			int sum = 0;			
			for (int j = 0; j < dst.rows; j++)			
			{				
				sum += col.data[j];			
			}			
			if (!sum)			
			{				
				col_count_end = i;				
				break;			
			}		
		} 		
		Mat tempImage;		
		if (col_count_begin == col_count_end)		
		{			
			tempImage = dst(Range::all(), Range(col_count_begin, dst.cols - 1));			
			end = true;		
		}		
		else		
		{
			tempImage = dst(Range::all(), Range(col_count_begin, col_count_end));  // 取出一个数字		
		} 		
		//cout << "(" << tempImage.rows << ", " << tempImage.cols << ")\t"; 		
		
		Mat resultImage;		
		matchTemplate(temp, tempImage, resultImage, 1);//temp是匹配图,tempImage是原图,result是结果/返回值,method表示比较所用的方法		
		double minValue, maxValue;		
		Point minLocation;		
		minMaxLoc(resultImage, &minValue, &maxValue, &minLocation); 		
		float multiple = 80.0 / dst.rows;		
		int num1 = 70;		
		int num2 = 115;		
		int num3 = 173;		
		int num4 = 228;		
		int num5 = 290;		
		int num6 = 348;		
		int num7 = 405;		
		int num8 = 465;		
		int num9 = 522; 		
		//cout << "minLocation.x = " << minLocation.x << "\t"; 		
		
		if (minLocation.x < cvRound(15 / multiple)) 
		{			
			cout << "0  ";		
		} 
		else if (minLocation.x > cvRound((num1 - 15) / multiple) && minLocation.x < cvRound((num1 + 15) / multiple)) 
		{			
			cout << "1  ";		
		} 
		else if (minLocation.x > cvRound((num2 - 15) / multiple) && minLocation.x < cvRound((num2 + 15) / multiple)) 
		{			
			cout << "2  ";		
		} 
		else if (minLocation.x > cvRound((num3 - 15) / multiple) && minLocation.x < cvRound((num3 + 15) / multiple)) 
		{			
			cout << "3  ";		
		} 
		else if (minLocation.x > cvRound((num4 - 15) / multiple) && minLocation.x < cvRound((num4 + 15) / multiple)) 
		{			
			cout << "4  ";		
		} 
		else if (minLocation.x > cvRound((num5 - 15) / multiple) && minLocation.x < cvRound((num5 + 15) / multiple)) 
		{			
			cout << "5  ";		
		} 
		else if (minLocation.x > cvRound((num6 - 15) / multiple) && minLocation.x < cvRound((num6 + 15) / multiple)) 
		{			
			cout << "6  ";		
		} 
		else if (minLocation.x > cvRound((num7 - 15) / multiple) && minLocation.x < cvRound((num7 + 15) / multiple)) 
		{			
			cout << "7  ";		
		} 
		else if (minLocation.x > cvRound((num8 - 15) / multiple) && minLocation.x < cvRound((num8 + 15) / multiple)) 
		{			
			cout << "8  ";		
		} 
		else if (minLocation.x > cvRound((num9 - 15) / multiple) && minLocation.x < cvRound((num9 + 15) / multiple)) 
		{			
			cout << "9  ";		
		} 
		else 
		{			
			cout << "ERROR  ";		
		}				
		for (int i = col_count_end + 1; i < dst.cols - 1; i++)		
		{			
			Mat col = dst.col(i).clone();			
			int sum = 0;			
			for (int j = 0; j < dst.rows; j++)			
			{				
				sum += col.data[j];			
			}			
			if (sum)			
			{				
				col_count_begin = i;			
				col_count_end = i;				
				break;			
			}		
		} 		
		if (end)			
			cout << endl << "----------------------------------" << endl;			
}
}