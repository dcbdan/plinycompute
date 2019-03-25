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
#ifndef PDB_PHYSICAL_OPTIMIZER_H
#define PDB_PHYSICAL_OPTIMIZER_H


#include "Computation.h"
#include "PDBVector.h"
#include <PDBLogger.h>
#include <AtomicComputationList.h>
#include <PDBPhysicalAlgorithm.h>
#include <PDBPipeNodeBuilder.h>
#include <PDBDistributedStorage.h>

namespace pdb {

using OptimizerSource = std::pair<size_t, PDBAbstractPhysicalNodePtr>;

/**
 * This class basically takes in a TCAP and breaks it up into PhysicalAlgorithms, that are going to be sent to,
 * the @see ExecutionServerFrontend
 */
class PDBPhysicalOptimizer {
public:

  /**
   * Takes in the TCAP string that we want to analyze and to the physical optimization on
   * @param computationID - the id of the computation this optimizer is to optimize
   * @param tcapString - the TACP string
   * @param clientPtr - the catalog client
   * @param logger - the logger
   */
  template <class CatalogClient>
  PDBPhysicalOptimizer(uint64_t computationID, String tcapString, const std::shared_ptr<CatalogClient> &clientPtr, PDBLoggerPtr &logger);

  /**
   * Default destructor
   */
  ~PDBPhysicalOptimizer() = default;

  /**
   * Returns the next algorithm we want to run.
   * Warning this assumes a that an allocating block has been setup previously
   * @return the algorithm - the algorithm we want to run
   */
  pdb::Handle<pdb::PDBPhysicalAlgorithm> getNextAlgorithm();

  /**
   * This returns true if there is an algorithm we need to run in order to finish the computation
   * @return true if there is false otherwise
   */
  bool hasAlgorithmToRun();

  /**
   * Updates the set statistics //TODO this needs to be implemented
   */
  void updateStats();

  /**
   * Returns the list of all the page sets that are scheduled to remove
   * @return the identifier of the page sets we need to remove, usually pairs of (computationID, tupleSetName)
   */
  std::vector<pair<uint64_t, std::string>> getPageSetsToRemove();

private:

  /**
   * The identifier of the computation
   */
  size_t computationID;

  /**
   * These are all the sources we currently have
   */
  priority_queue<OptimizerSource, vector<OptimizerSource>, function<bool(const OptimizerSource&, const OptimizerSource&)>> sources;

  /**
   * The logger associated with the physical optimizer
   */
  PDBLoggerPtr logger;

};

}

#include <PDBPhysicalOptimizerTemplate.cc>

#endif //PDB_PHYSICAL_OPTIMIZER_H