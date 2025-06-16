#include "CImg.h"
#include "HybridBinarizer.h"
#include "LuminanceSource.h"
#include "Counted.h"
#include "BitMatrix.h"
#include "DecodeHints.h"
#include "Detector.h"
#include "zxblocks.h"
#include "zxdecoderlib.h"
#include "PiCodeDecoder.h"
#include <time.h>

using namespace cimg_library;
using namespace std;
using namespace zxing;

	using zxing::ArrayRef;
	using zxing::HybridBinarizer;
	using zxing::LuminanceSource;
	using zxing::datamatrix::Detector;

string PiCodeDecoder::picodedecode(char* filename)
{
	unsigned char * R = NULL;
	unsigned char * G = NULL;
	unsigned char * B = NULL;
	unsigned char* GREY = NULL;
	unsigned char* ReGREY = NULL;
	string decodedmsg;

	try {
		CImg<unsigned char> imgLogo(filename);
		int width = imgLogo.width();
		int height = imgLogo.height();

		GREY = new unsigned char[width*height];
		R = imgLogo.data(0, 0, 0, 0);
		G = imgLogo.data(0, 0, 0, 1);
		B = imgLogo.data(0, 0, 0, 2);
		
		for(int idx = 0; idx < width * height; idx++)
		{
			GREY[idx]=(unsigned char)(0.299*(double)R[idx]+0.587*(double)G[idx]+0.114* (double)B[idx]);
		}

		ReGREY = bilinearresize(GREY, width, height, 600, 600);
		
		decodedmsg = picode(ReGREY, 600, 600);

		// Clean up allocated memory
		delete[] GREY;
		delete[] ReGREY;

		if(decodedmsg.length() > 0)
			return decodedmsg;
		else
			return "Can not detect or decode the PiCode, please try another image...";

	} catch (const CImgException &e) {
		// Clean up memory in case of an exception
		delete[] GREY;
		delete[] ReGREY;
		std::cerr << "A CImg exception occurred in picodedecode: " << e.what() << std::endl;
		return "Failed to process image file.";
	} catch (...) {
		// Clean up memory in case of an exception
		delete[] GREY;
		delete[] ReGREY;
		std::cerr << "An unknown exception occurred in picodedecode." << std::endl;
		return "An unknown error occurred during decoding.";
	}
}

string	PiCodeDecoder::picode (unsigned char* greydata, int width, int height)
{

	string msg="";
/*
		time_t t= time(0);
	struct tm *now =localtime(& t);
		if (now->tm_year+1900>2014)
			return msg;
*/
    Ref<LuminanceSource> source(new LuminanceSource(greydata,width,height));
   	Ref<HybridBinarizer> binarizer(new HybridBinarizer(source));


	Detector detector(binarizer->getBlackMatrix());

	try
	{
	    Ref<DetectorResult> detectorResult(detector.detect());
	    	ArrayRef< Ref<ResultPoint> > resultpoints(detectorResult->getPoints());

	    	Ref<BitMatrix> resultbits(detectorResult->getBits());

	    	int dimension=400;
	        Ref<PerspectiveTransform> transform(detector.createTransform(resultpoints[0],resultpoints[2],resultpoints[1],resultpoints[3],dimension,dimension));

	        	unsigned char** bitmap = new unsigned char*[dimension];
	        		       	for(int x=0;x<dimension;x++)
	        		       	{
	        		       		bitmap[x] = new unsigned char[dimension];
	        		       	}
	          vector<float> points(dimension << 1, (const float)0.0f);
	          for (int y = 0; y < dimension; y++) {
	            int max = points.size();
	            float yValue = (float)y + 0.5f;
	            for (int x = 0; x < max; x += 2) {
	              points[x] = (float)(x >> 1) + 0.5f;
	              points[x + 1] = yValue;
	            }
	            transform->transformPoints(points);
	            for (int x = 0; x < max; x += 2) {

	              bitmap[x>>1][y]=greydata[((int)points[x+1])*width+(int)points[x]];
	            }
	          }

	        int modules= detectorResult->getModules();

	       	
	       	try
	       	{
	       		msg = decode(bitmap, resultbits, dimension, dimension,modules);
	       	}
	       	catch (...)
	       	{
				
	       		return msg;
	       	}

	      

	       	for(int i=0;i<dimension;i++)
	       	{
	       		delete[] bitmap[i];
	       	}
	       	delete[] bitmap;

	       	return msg;
	}
	catch(...)
	{
		 
		  return msg;
	}

}

unsigned char* PiCodeDecoder::bilinearresize(unsigned char in[],
    int src_width, int src_height, int dest_width, int dest_height)
{
    unsigned char* out=new unsigned char[dest_width * dest_height];

    const float ty = float(src_width-1) / (dest_width-1);
    const float tx = float(src_height-1) / (dest_height-1);

	for (int i=0;i<dest_height;i++)
	{
			for(int j=0;j<dest_width;j++)
		{
		    float old_height=i*tx;
			float old_width =j*ty;
			int   old_height_i=floor(old_height);
			int   old_width_i=floor(old_width);
			float lt=in[old_height_i*src_width+old_width_i]*(1-old_height+old_height_i)*(1-old_width+old_width_i);
			float ld=in[(old_height_i+1)*src_width+old_width_i]*(old_height-old_height_i)*(1-old_width+old_width_i);
			float rt=in[old_height_i*src_width+old_width_i+1]*(1-old_height+old_height_i)*(old_width-old_width_i);
			float rd=in[(old_height_i+1)*src_width+old_width_i+1]*(old_height-old_height_i)*(old_width-old_width_i);
			out[i*dest_width+j]=(unsigned char)(lt+ld+rt+rd);
		}
	}

    return out;
}



string decode_picode(char* filename)
{
	PiCodeDecoder pc;
	string flag = pc.picodedecode(filename);
	
	return flag; //0: successful; 1: message too long/short; otherwise: failure.
}// fxn

