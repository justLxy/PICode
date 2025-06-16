
#include "GlobalHistogramBinarizer.h"
//#include "IllegalArgumentException.h"
#include "Array.h"

namespace zxing {
using namespace std;

const int LUMINANCE_BITS = 5;
const int LUMINANCE_SHIFT = 8 - LUMINANCE_BITS;
const int LUMINANCE_BUCKETS = 1 << LUMINANCE_BITS;

GlobalHistogramBinarizer::GlobalHistogramBinarizer(Ref<LuminanceSource> source) :
  Binarizer(source), cached_matrix_(NULL), cached_row_(NULL), cached_row_num_(-1) {

}

GlobalHistogramBinarizer::~GlobalHistogramBinarizer() {
}



Ref<Binarizer> GlobalHistogramBinarizer::createBinarizer(Ref<LuminanceSource> source) {
  return Ref<Binarizer> (new GlobalHistogramBinarizer(source));
}

}
