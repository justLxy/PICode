// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
#ifndef __LUMINANCESOURCE_H__
#define __LUMINANCESOURCE_H__
/*
 *  LuminanceSource.h
 *  zxing
 *
 *  Copyright 2010 ZXing authors All rights reserved.
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

#include "Counted.h"
#include <string.h>

namespace zxing {

class LuminanceSource : public Counted {


  private:
  unsigned char* greyData_;
  
  int width_;
  int height_;

 public:
  LuminanceSource(unsigned char* greyData, int width, int height);

  unsigned char* getRow(int y, unsigned char* row);
  unsigned char* getMatrix();

  

  int getWidth() const {
    return width_;
  }

  int getHeight() const {
    return height_;
  }

 


                           // large breaking change right now
};

}

#endif /* LUMINANCESOURCE_H_ */
