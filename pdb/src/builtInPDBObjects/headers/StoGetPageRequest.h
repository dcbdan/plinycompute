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

#ifndef CAT_STO_GET_PAGE_REQ_H
#define CAT_STO_GET_PAGE_REQ_H

#include "Object.h"
#include "Handle.h"
#include "PDBString.h"
#include "PDBSet.h"

// PRELOAD %StoGetPageRequest%

namespace pdb {

// encapsulates a request to obtain a type name from the catalog
class StoGetPageRequest : public Object {

public:

  StoGetPageRequest() = default;

  ~StoGetPageRequest() = default;

  StoGetPageRequest(const pdb::PDBSetPtr &whichSet, uint64_t pageNumber) : pageNumber(pageNumber) {


    if(whichSet != nullptr){

      setName = whichSet->getSetName();
      dbName = whichSet->getDBName();
      isAnon = false;
    }
    else {
      setName = "";
      dbName = "";
      isAnon = true;
    }
  }


  ENABLE_DEEP_COPY

  /**
   * The name of the set we are requesting the page for
   */
  String setName;

  /**
   * The name of the database we are requesting the page for
   */
  String dbName;

  /**
   * The page number
   */
  uint64_t pageNumber = 0;

  /**
   * Is this an anonymous page
   */
  bool isAnon;
};
}

#endif
