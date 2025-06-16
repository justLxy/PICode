/*
Functions and types that implements Otsu's image thresholding algorithm.
Author: Alan Baojian ZHOU
Created: 20150326
Last-Modified: 20150326
*/

#ifndef _OTSU_THRESH_H
#define _OTSU_THRESH_H


/* Data structure that stores the result */
struct _ThresholdResult
{
	unsigned int num;			// Number of pixels
	unsigned int numBackground;	// Number of pixels in background
	unsigned int numForeground;	// Number of pixels in foreground
	unsigned int sum;			// Sum of pixel value
	unsigned int sumBackground;	// Sum of pixel value in background
	unsigned int sumForeground;	// Sum of pixel value in foreground
	float meanBackground;		// Mean of pixel value in background
	float meanForeground;		// Mean of pixel value in foreground
	float threshold;			// Threshold
	float interClassVariance;	// Inter-class variance
};
typedef struct _ThresholdResult ThresholdResult;


/* Functions */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


void GetHistogram(unsigned int *histogram, unsigned int numBin,
	unsigned int *image, unsigned int numPixel);

ThresholdResult GetThreshold(unsigned int *histogram, unsigned int numBin);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _OTSU_THRESH_H