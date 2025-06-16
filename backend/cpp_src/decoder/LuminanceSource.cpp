// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  LuminanceSource.cpp
 *  zxing
 *
 *  Copyright 2008 ZXing authors All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sstream>
#include "LuminanceSource.h"
//#include "IllegalArgumentException.h"

namespace zxing {






LuminanceSource::LuminanceSource(unsigned char* greyData, int width, int height) : greyData_(greyData),
    width_(width),
    height_(height) {

  
}

unsigned char* LuminanceSource::getRow(int y, unsigned char* row) {
 
  int width = getWidth();
  // TODO(flyashi): determine if row has enough size.
  if (row == NULL) {
    row = new unsigned char[width_];
  }
  int offset = y  * width;
  memcpy(row, &greyData_[offset], width);
  return row;
}

unsigned char* LuminanceSource::getMatrix() {
  int size=width_*height_;
  unsigned char* result = new unsigned char[size];
	memcpy(result, greyData_, size);
  return result;
}




}
