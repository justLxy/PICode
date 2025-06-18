#ifndef _PICODE_ENCODER_H
#define _PICODE_ENCODER_H

// Enable JPEG, PNG, and TIFF support in CImg.h.
#define cimg_use_jpeg CIMG_USE_JPEG
#define cimg_use_png CIMG_USE_PNG
//#define cimg_use_tiff CIMG_USE_TIFF

#include "CImg.h"
using namespace cimg_library;

class PiCodeEncoder
{
public:
	PiCodeEncoder(void);
	~PiCodeEncoder(void);
	// Variables
public:
	// data header
	static const unsigned int versionLength = 3; // k of BCH(n,k)
	static const unsigned int msgLength = 11; // k of BCH(n,k)
	static const unsigned int headerLength = 53; // 53 bit header
	// Truncated BCH(63,24) -> BCH(53,14)
	static const unsigned int headerTruncatedLength = 10; // Truncated length
	static const unsigned int headerBCHLength = 63; // BCH code length before truncated
	static const unsigned int headerBCHm = 6; // GF size order GF(2^m)
	static const unsigned int headerBCHt = 7; // BCH error correction capability t

public:
	unsigned int version; // Version number

	unsigned int moduleSize; // Number of pixels in one module
	unsigned int moduleNumData;// Number of modules in the data region (along one dimension )
	unsigned int moduleNumFP; // Number of modules in (one-side of) the finder pattern
	unsigned int moduleNumQZ; // Number of modules in (one-size of) the quiet zone

	unsigned char numStart; // Smallest unsigned char encoding for numbers
	unsigned char numEnd; // Greatest unsigned char encoding for numbers
	unsigned char charStart; // Smallest supported char character

	unsigned int RSGFPoly; // Use GF(2^8) array. Primitive polynomial = D^8+D^4+D^3+D^2+1 (285 decimal)
	float codeRateMax; // Maximum code rate
	
	float quality; // A quality factor that controls image quality after modulation. i.e., TOL in Lewis' notation.

//  Functions
public:
	// Interface function. Alan ZHOU 20140223
	// generate and save a picode to the current directory
	int GeneratePiCode(char msg[], char filenameLogo[], char filenamePiCode[]);
	int GeneratePiCode(char msg[], char filenameLogo[], char filenamePiCode[], unsigned int moduleNumData, unsigned int moduleSize, unsigned int quality);
	int GeneratePiCode(char *msg, int *logoPixels, int *picodePixels);
private:
	// Lower level functions
	int Encode(char *msg, char *data);
	bool SourceCoding(char *msg, unsigned char * &msgBytes, unsigned int &msgBytesLength);
	int RSCoding(unsigned char *msgBytes, unsigned int msgBytesLength, unsigned char * &codeBytes, unsigned int &codeBytesLength);
	bool AssembleData(unsigned int msgBytesLength, unsigned char *codeBytes, unsigned int codeBytesLength, char *data);
	bool Interleave(char *data);

	bool Embed(char *data, CImg<unsigned int> &imgLogo, CImg<unsigned int> &imgPiCode);
	bool Embed(char *data, int *logoPixels, int *picodePixels);
	bool EmbedData(char *data, float *logoPixelsY);
	float CalculateEnergyRatio(float *logoPixelsY, unsigned int idxModule);
	bool AttachFinderPattern(CImg<unsigned int> &imgPiCode);
	bool AttachFinderPattern(int *picodePixels);
	bool FillDataRegion(CImg<unsigned int> &imgLogo, CImg<unsigned int> &imgPiCode);
	bool FillDataRegion(int *logoPixels, int *picodePixels);
	bool GetTilingPattern(unsigned char* tilingPattern, char moduleValue);

	bool Beautify(int *logoPixels);
};

// Global variables
// High frequency trings
static const unsigned int hfNum = 9; // Number of high frequency strings to be considered
static const char hfStringArray[hfNum][100] = {"ust.hk",  "www.", ".com", ".org", "http://", 
	"https://", "facebook", "renren", "eewhmow"};
static const unsigned char hfByteArray[hfNum] = {95, 96, 97, 98, 99, 100, 101, 102, 103};


// Interface function with C symbols for PHP.
// generate and save a picode to the current directory
extern "C" int generate_picode(char msg[], char filenameLogo[], char filenamePiCode[], unsigned int moduleNumData, unsigned int moduleSize, unsigned int quality);

#endif

