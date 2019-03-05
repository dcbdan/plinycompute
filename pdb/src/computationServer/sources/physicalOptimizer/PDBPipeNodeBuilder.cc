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

#include <physicalOptimizer/PDBJoinPhysicalNode.h>
#include "physicalOptimizer/PDBPipeNodeBuilder.h"
#include "physicalOptimizer/PDBStraightPhysicalNode.h"
#include "physicalOptimizer/PDBAggregationPhysicalNode.h"
#include "AtomicComputationList.h"


namespace pdb {

PDBPipeNodeBuilder::PDBPipeNodeBuilder(size_t computationID, const std::shared_ptr<AtomicComputationList> &computations)
    : atomicComps(computations), currentNodeIndex(0), computationID(computationID) {}
}

std::vector<pdb::PDBAbstractPhysicalNodePtr> pdb::PDBPipeNodeBuilder::generateAnalyzerGraph() {

  // go through each source in the sources
  for(const AtomicComputationPtr &source : atomicComps->getAllScanSets()) {

    // go trough each consumer of this node
    for(const auto &consumer : atomicComps->getConsumingAtomicComputations(source->getOutputName())) {

      // we start with a source so we push that back
      currentPipe.push_back(source);

      // add the consumer to the pipe
      currentPipe.push_back(consumer);

      // then we start transversing the graph upwards
      transverseTCAPGraph(consumer);
    }
  }

  // connect the pipes
  connectThePipes();

  // return the generated source nodes
  return this->physicalSourceNodes;
}

void pdb::PDBPipeNodeBuilder::transverseTCAPGraph(AtomicComputationPtr curNode) {

  // did we already visit this node
  if(visitedNodes.find(curNode) != visitedNodes.end()) {

    // clear the pipe we are done here
    currentPipe.clear();

    // we are done here
    return;
  }

  // ok now we visited this node
  visitedNodes.insert(curNode);

  // check the type of this node might be a pipeline breaker
  switch (curNode->getAtomicComputationTypeID()) {

    case HashOneTypeID:
    case HashLeftTypeID:
    case HashRightTypeID: {

      // we got a hash operation, create a PDBJoinPhysicalNode
      createPhysicalPipeline<PDBJoinPhysicalNode>();
      currentPipe.clear();

      break;
    }
    case ApplyAggTypeID: {

      // we got a aggregation so we need to create an PDBAggregationPhysicalNode
      // we need to remove the ApplyAgg since it will be run in the next pipeline this one is just preparing the data for it.
      currentPipe.pop_back();
      createPhysicalPipeline<PDBAggregationPhysicalNode>();

      // add the ApplyAgg back to an empty pipeline
      currentPipe.clear();
      currentPipe.push_back(curNode);

      break;
    }
    case WriteSetTypeID: {

      // write set also breaks the pipe because this is where the pipe ends
      createPhysicalPipeline<pdb::PDBStraightPhysicalNode>();
      currentPipe.clear();
    }
    default: {

      // we only care about these since they tend to be pipeline breakers
      break;
    }
  }

  // grab all the consumers
  auto consumers = atomicComps->getConsumingAtomicComputations(curNode->getOutputName());

  // if we have multiple consumers and there is still stuff left in the pipe
  if(consumers.size() > 1 && !currentPipe.empty()) {

    // this is a pipeline breaker create a pipe
    //currentPipe.push_back(curNode);
    createPhysicalPipeline<PDBStraightPhysicalNode>();
    currentPipe.clear();
  }

  // go through each consumer and transverse to get the next pipe
  for(auto &consumer : consumers) {
    currentPipe.push_back(consumer);
    transverseTCAPGraph(consumer);
  }
}

void pdb::PDBPipeNodeBuilder::setConsumers(std::shared_ptr<PDBAbstractPhysicalNode> node) {

  // all the consumers of these pipes
  std::vector<std::string> consumers;

  // go trough each consumer of this node
  for(const auto &consumer : atomicComps->getConsumingAtomicComputations(this->currentPipe.back()->getOutputName())) {

    // if the next pipe begins with a write set we just ignore it...
    // this is happening usually when we have an aggregation connected to a write set which is not really necessary
    if(consumer->getAtomicComputationTypeID() == WriteSetTypeID){
      continue;
    }

    // add them to the consumers
    consumers.push_back(consumer->getOutputName());
  }

  // set the consumers
  if(!consumers.empty()) {
    this->consumedBy[node->getNodeIdentifier()] = consumers;
  }
}

void pdb::PDBPipeNodeBuilder::connectThePipes() {

  for(const auto &node : physicalNodes) {

    // get all the consumers of this pipe
    auto consumingAtomicComputation = consumedBy[node.second->getNodeIdentifier()];

    // go through each at
    for(const auto &atomicComputation : consumingAtomicComputation) {

      // get the consuming pipeline
      auto consumer = startsWith[atomicComputation];

      // add the consuming node of this guy
      node.second->addConsumer(consumer);
    }
  }
}