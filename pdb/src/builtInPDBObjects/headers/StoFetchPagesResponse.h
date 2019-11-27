/*****************************************************************************
 *                                                                           *
 *  Copyright 2018 Rice University                                           *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *      http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 *****************************************************************************/

#ifndef KEEP_GOING_H
#define KEEP_GOING_H

#include "Object.h"

//  PRELOAD %StoFetchPagesResponse%

namespace pdb {

class StoFetchPagesResponse : public Object {
public:

  StoFetchPagesResponse() = default;
  explicit StoFetchPagesResponse(uint64_t numPages) : numPages(numPages) {}

  ENABLE_DEEP_COPY

  uint64_t numPages = 0;
};
}

#endif
