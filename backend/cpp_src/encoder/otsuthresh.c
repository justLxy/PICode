/*
Functions and types that implements Otsu's image thresholding algorithm.
Author: Alan Baojian ZHOU
Created: 20150326
Last-Modified: 20150326
*/

#include "otsuthresh.h"
#include <string.h>

/*
Get the histogram of a rectangle area in an image.
Parameters:
histogram - array containing the histogram.
	Note: Memory should be allocated and managed by caller.
numBin - number of bins, i.e., the maximum pixel value, e.g., 256 for a gray scale image.
image - array that stores the pixel values of the image with pixel order
	left-to-right, top-to-bottom.
	Note: Memory should be allocated and managed by caller.
numPixel - number of pixels in the image.
*/
void GetHistogram(unsigned int *histogram, unsigned int numBin,
	unsigned int *image, unsigned int numPixel)
{
	// Construct the histogram.
	unsigned int idx = 0; // Index of pixel
	memset(histogram, 0, sizeof(unsigned int) * numBin); // Set all data to 0.
	for (idx = 0; idx < numPixel; ++idx)
	{
			++histogram[image[idx]];
	}
} // fxn GetHistogram

/*
Get the threshold result with Otsu's method for a given histogram.
Parameters:
histogram - array containing the histogram.
	Note: Memory should be allocated and managed by caller.
numBin - number of bins, i.e., the maximum pixel value, e.g., 256 for a gray scale image.
Return:
A ThresholdResult object that contains the thresholding result.
*/
ThresholdResult GetThreshold(unsigned int *histogram, unsigned int numBin)
{
	// Determine the threshold using Otsu's method.
	// Thresholding result
	ThresholdResult tr;
	// Temporary variables
	unsigned int num;			// Number of pixels
	unsigned int numBackground;	// Number of background pixels
	unsigned int numForeground;	// Number of foreground pixels
	unsigned int sum;			// Sum of pixel values
	unsigned int sumBackground;	// Sum of background pixel values
	unsigned int sumForeground; // Sum of foreground pixel values
	float meanBackground;		// Mean of background pixel values
	float meanForeground;		// Mean of foreground pixel values
	float interClassVariance;	// Inter-class variance of pixel values
	unsigned char thresholdGE;	// Threshold with ">=" condition
	unsigned char thresholdG;	// Threshold with ">" condition
	unsigned int i;				// Pixel value as traverse variable
	// Initialize
	tr.num = 0;
	tr.numBackground = 0;
	tr.numForeground = 0;
	tr.sum = 0;
	tr.sumBackground = 0;
	tr.sumForeground = 0;
	tr.meanBackground = 0;
	tr.meanForeground = 0;
	tr.threshold = 0;
	tr.interClassVariance = 0;

	num = 0;
	numBackground = 0;
	numForeground = 0;
	sum = 0;
	sumBackground = 0;
	sumForeground = 0;
	meanBackground = 0;
	meanForeground = 0;
	interClassVariance = 0;
	thresholdGE = 0;
	thresholdG = 0;

	// First get the sum.
	for (i = 1; i < numBin; ++i)
	{
		num += histogram[i];
		sum += i * histogram[i];
	}
	tr.num = num;
	tr.sum = sum;

	// Then traverse all the pixel values to find the best threshold.
	for (i = 0; i < numBin; ++i)
	{
		numBackground += histogram[i];
		if (numBackground == 0)
		{
			continue;
		}

		numForeground = num - numBackground;
		if (numForeground == 0)
		{
			break;
		}

		sumBackground += i * histogram[i];
		sumForeground = sum - sumBackground;
		meanBackground = (float)sumBackground / numBackground;
		meanForeground = (float)sumForeground / numForeground;
		interClassVariance = numBackground * numForeground *
			(meanBackground - meanForeground) * (meanBackground - meanForeground);

		if (interClassVariance >= tr.interClassVariance)
		{
			thresholdGE = i;
			tr.numBackground = numBackground;
			tr.numForeground = numForeground;
			tr.sumBackground = sumBackground;
			tr.sumForeground = sumForeground;
			tr.meanBackground = meanBackground;
			tr.meanForeground = meanForeground;
			if (interClassVariance > tr.interClassVariance)
			{
				thresholdG = i;
				tr.interClassVariance = interClassVariance;
			}
		}
	} // for i
	tr.threshold = (float)(thresholdGE + thresholdG) / 2;

	return tr;
} // fxn GetThreshold