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
#ifndef PDB_TCAPPARSER_TCAPPARSER_H
#define PDB_TCAPPARSER_TCAPPARSER_H

#include <memory>
#include <string>

#include "SafeResult.h"
#include "TranslationUnit.h"

using std::shared_ptr;
using std::string;

using pdb::SafeResult;

namespace pdb_detail {
/**
 * Parses the given TCAP program string into a TranslationUnit.
 *
 * @param source a program in the TCAP language
 * @return SafeResultSuccess if the string parsed, else a SafeResultFailure.
 */
shared_ptr<SafeResult<TranslationUnit>> parseTcap(const string& source);
}

#endif  // PDB_TCAPPARSER_TCAPPARSER_H