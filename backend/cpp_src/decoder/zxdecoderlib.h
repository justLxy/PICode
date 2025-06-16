#include "BitMatrix.h"
#include "zxblocks.h"
using zxing::BitMatrix;
using namespace std;

string decode(unsigned char**, zxing::BitMatrix*, int, int,int);
int* examineBorder(bool, int, int, int, BitMatrix*, int, int);
int countHeader (Blocks*, int*, int*);
int numDataCwToTotalBit (int);
int numofCw(int);
string toString (int);

