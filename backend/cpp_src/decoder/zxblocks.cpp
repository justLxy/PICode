#include <time.h>
#include <cmath>
#include <algorithm>
#include "zxblocks.h"

//Construtor for blocks object
Blocks::Blocks (unsigned char** Bm, int numModules, int bmsize)
{
	DecodeBm = Bm;
	imgWidth = bmsize;
	imgHeight = bmsize;

	totalModules = numModules * numModules;

	erasurePosition = new int[totalModules];
	for(int i=0;i<totalModules;i++)
		erasurePosition[i] = 0;

	allBlkDiff = new int[totalModules];
	for(int i=0;i<totalModules;i++)
		allBlkDiff[i] = 256;		//index with element 256 means they are not data+ecc

	allBlkOrder = new int[totalModules];
	for(int i=0;i<totalModules;i++)
		allBlkOrder[i] = 0;

	numErasure = 0;
}

//Destructor for blocks object
Blocks::~Blocks()
{
	//Memory release
	delete[] erasurePosition;
	delete[] allBlkDiff;
	delete[] allBlkOrder;
}

//Decode one block from the Bitmap
bool Blocks::DecodeBlock(int BorderXStart, int BorderXEnd, int BorderYStart, int BorderYEnd, int blockOrder)
{
	//Get the scaled block
	unsigned char** blockBm = getBlockFromBm(BorderXStart, BorderXEnd, BorderYStart, BorderYEnd);

	int width = scaledSize;
	int height = scaledSize;
	blockCenter = 0;
	blockEdge = 0;

	//Decode the scaled block into true/false
	bool result = decodeScaledBlock(blockBm, blockOrder);

	//Memory release
	for(int i=0;i<width;i++)
		delete[] blockBm[i];
	delete[] blockBm;

	return result;
}

//Crop and resize one block from bitmap
unsigned char** Blocks::getBlockFromBm(int BorderXStart, int BorderXEnd, int BorderYStart, int BorderYEnd)
{
	if(BorderXEnd-BorderXStart+1 < 0 || BorderYEnd-BorderYStart+1 < 0)
		return 0;

	//Crop one block from Bitmap
	int cropWidth = BorderXEnd-BorderXStart+1;
	int cropHeight = BorderYEnd-BorderYStart+1;

	unsigned char** cropBitmap = new unsigned char*[cropWidth];

	for(int x=0;x<cropWidth;x++)
	{
		cropBitmap[x] = new unsigned char[cropHeight];
		for(int y=0;y<cropHeight;y++)
		{
			int px = BorderXStart+x;
			if (px >= imgWidth)
				px = imgWidth - 1;
			int py = BorderYStart+y;
			if (py >= imgWidth)
				py = imgWidth - 1;
			cropBitmap[x][y] = DecodeBm[px][py];
		}
	}

	//Resize the cropped block
	unsigned char** scaledBitmap = resize(cropBitmap, cropWidth, cropHeight, scaledSize, scaledSize);

	//Memory release
	for(int x=0;x<cropWidth;x++)
		delete[] cropBitmap[x];
	delete[] cropBitmap;

	//Return the scaled block
	return scaledBitmap;
}

//Decode one single block into true/false
//Refer to Java version for details
bool Blocks::decodeScaledBlock(unsigned char** blockBm, int blockOrder)
{
	int width = scaledSize;
	int height = scaledSize;

	int length = width*height;
	int* value=new int [length];

	for(int i=0;i<width;i++)
		for(int j=0;j<height;j++)
			value[i*width+j] = blockBm[i][j];

/*	for(int i=0; i<length; i++){
		int result=demodule(blockCenter,blockEdge,value[i],i);
		blockCenter =result/10000;
		blockEdge=result%10000;
	}
	blockCenter >>= 2;
	blockEdge /= 20;
*/

	for(int i=0; i<length; i++)
	{
		value[i] &= 0xff;
		if(i == 27 || i == 28 || i == 35 || i==36)
		{
			  blockCenter += value[i];
		}
		else if( (i >= 9 && i <= 14) || (i == 17) || (i==22) || (i==25) || (i==30) || (i==33) || (i==38) || (i == 41) || (i == 46) || (i >= 49 && i <= 54))
		{
			  blockEdge += value[i];
		}

	}
	blockCenter >>= 2;
	blockEdge /= 20;



	blockDiff = abs(blockCenter - blockEdge);

	if(blockOrder != 999){		//these are bytes of data+ecc
		allBlkDiff[blockOrder] = blockDiff;
	}
	//erasure detection, 999 means no need to mark erasure for the block
	if(blockDiff < 10 && blockOrder != 999 && numErasure < erasureLimit){
		int byteOrder = (blockOrder >> 3) + 1;	//Get the corresponding byte no.
		int erasureAddress = totalByte - byteOrder;

		if(numErasure == 0){										//First byte of header+data+ecc
			erasurePosition[numErasure++] =  erasureAddress;
		}else if(erasurePosition[numErasure-1] != erasureAddress){	//Make sure the byte is not marked before
			erasurePosition[numErasure++] =  erasureAddress;
		}
	}

	//shun method (for edge block)
	if(blockDiff < edgeThreshold){
		int edgeArea[20] = {value[9],value[10],value[11],value[12],value[13],value[14],value[49],value[50],value[51],value[52],
				value[53],value[54],value[17],value[22],value[25],value[30],value[33],value[38],value[41],value[46]};
		int edgeLength = 20;

		std::sort(edgeArea, edgeArea+20);

		int edgeDiff = edgeArea[edgeLength - 1] - edgeArea[0];
		if(edgeDiff > 100){
			int centerPoint = edgeArea[0] + (edgeDiff >> 1);
			int		edgeRightLength = 0;	int	edgeLeftLength = 0;
			int 	sumRight = 0;			int	sumLeft = 0;
			for(int i=0;i<edgeLength;i++){
				if(edgeArea[i] > centerPoint){
					edgeRightLength++;
					sumRight += edgeArea[i];
				}else if (edgeArea[i] < centerPoint){
					edgeLeftLength++;
					sumLeft += edgeArea[i];
				}
			}

			if(edgeLeftLength > edgeRightLength)
				blockEdge = sumLeft / edgeLeftLength;
			else if (edgeRightLength > edgeLeftLength)
				blockEdge = sumRight / edgeRightLength;
		}
	}

	if(blockCenter > blockEdge)
		return true;
	else
		return false;
}

//Refer to Java version for details
/*int Blocks::demodule(int a, int b, int value, int i)
{
	time_t t= time(0);
	struct tm *now =localtime(& t);
	if (now->tm_year+1900<2014)
	{
		if(i == 27 || i == 28 || i == 35 || i==36)	//block center
			a += value;
		else if( (i >= 9 && i <= 14) || (i == 17) || (i==22) || (i==25) || (i==30) ||	//block edge
				(i==33) || (i==38) || (i == 41) || (i == 46) || (i >= 49 && i <= 54))
			b += value;
	}

	return a*10000+b;
}*/

//Resize Bitmap with Bilinear interpolation algorithm
unsigned char** Blocks::resize(unsigned char** src, int srcWidth, int srcHeight, int dstWidth, int dstHeight)
{
	//Construct the scaled Bitmap
	unsigned char** dst = new unsigned char*[dstWidth];
	for(int i=0;i<dstWidth;i++)
		dst[i] = new unsigned char[dstHeight];

	//Get the ratio of original Bitmap against scaled Bitmap
    double wratio = (double)srcWidth/(double)dstWidth;
    double hratio = (double)srcHeight/(double)dstHeight;

    //Loop through each pixel in the scaled Bitmap and construct the image
    for (int i = 0; i < dstWidth; i ++)
    {
		for (int j = 0; j < dstHeight; j ++)
		{
			//Get the xy-coordinate of the pixel in the original Bitmap
			double newx = i*wratio;
			double newy = j*hratio;

			//Use bilinear interpolation to estimate the value of the pixel

			//Get the xy-coordinate of the nearest 4 pixels in the original Bitmap
			int x1 = newx;
			int x2 = (newx + 1);
			int y1 = newy;
			int y2 = (newy + 1);

			if(x1>=srcWidth)
				x1 = srcWidth-1;
			if(x2>=srcWidth)
				x2 = srcWidth-1;
			if(y1>=srcHeight)
				y1 = srcHeight-1;
			if(y2>=srcHeight)
				y2 = srcHeight-1;

			//Get the value of the nearest 4 pixels
			double p1 = src[x1][y1];
			double p2 = src[x1][y2];
			double p3 = src[x2][y1];
			double p4 = src[x2][y2];

			//Convert the xy-coordinate into double for calculation
			//(Avoid truncation in integer arithmetic)
			double sx1 = x1;
			double sx2 = x2;
			double sy1 = y1;
			double sy2 = y2;

			//Linear interpolation of p1 and p2 to get p5
			//(x1, y1) vs (x1, y2)
			double m1 = (p2-p1)/(sy2-sy1);
			double b1 = p1 - m1*sy1;
			double p5 = m1*newy + b1;

			//Linear interpolation of p3 and p4 to get p6
			//(x2, y1) vs (x2, y2)
			double m2 = (p4-p3)/(sy2-sy1);
			double b2 = p3 - m2*sy1;
			double p6 = m2*newy + b2;

			//Linear interpolation of p5 and p6 to get the final estimation
			//p5 vs p6
			double m3 = (p6-p5)/(sx2-sx1);
			double b3 = p5 - m3*sx1;
			double final = m3*newx + b3;

			//Set the result to the scaled bitmap (int to char casting)
			dst[i][j] = final;

			//Some other approach and testing data suite for reference

			//Nearest 4 matches
			//dst[i][j] = (p1 + p2 + p3 + p4)/4;

			//Nearest match
			//int x = newx;
			//int y = newy;
			//dst[i][j] = src[x][y];

			//__android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "newx = %f, newy = %f", newx, newy);
			//__android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "x1 = %d, x2 = %d, y1 = %d, y2 = %d", x1, x2, y1, y2);
			//__android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "p1 = %f, p2 = %f, p3 = %f, p4 = %f", p1, p2, p3, p4);
			//__android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "Bilinear interpolation");
			//__android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "m1 = %f, b1 = %f, p5 = %f", m1, b1, p5);
			//__android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "m2 = %f, b2 = %f, p6 = %f", m2, b2, p6);
			//__android_log_print(ANDROID_LOG_ERROR, "JNIMsg", "m3 = %f, b3 = %f, final = %f", m3, b3, final);
		}
    }

    return dst;
}
