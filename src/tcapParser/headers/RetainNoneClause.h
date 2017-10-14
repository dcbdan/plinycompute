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
#ifndef PDB_RETAINNONECLAUSE_H
#define PDB_RETAINNONECLAUSE_H

#include "RetainClause.h"

namespace pdb_detail {
/**
 * A retention clause that indicates that no columns should be copied into the output table.
 */
class RetainNoneClause : public RetainClause {
    /**
     * @return false
     */
    bool isAll();

    /**
     * @return true
     */
    bool isNone();

    // contract from super
    void match(function<void(RetainAllClause&)> forAll,
               function<void(RetainExplicitClause&)>,
               function<void(RetainNoneClause&)> forNone);
};
}

#endif  // PDB_RETAINNONECLAUSE_H
