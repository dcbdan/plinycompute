#include <utility>

#include <utility>

//
// Created by dimitrije on 4/11/19.
//

#include <JoinBroadcastPipeline.h>
#include <pipeline/JoinBroadcastPipeline.h>
#include <MemoryHolder.h>

pdb::JoinBroadcastPipeline::JoinBroadcastPipeline(size_t workerID, size_t nodeID,
                                              pdb::PDBAnonymousPageSetPtr outputPageSet,
                                              pdb::PDBAbstractPageSetPtr inputPageSet,
                                              pdb::ComputeSinkPtr merger) : workerID(workerID), nodeID(nodeID), outputPageSet(std::move(outputPageSet)), inputPageSet(std::move(inputPageSet)), merger(std::move(merger)) {}


void pdb::JoinBroadcastPipeline::run() {

  // this is where we are outputting all of our results to
  MemoryHolderPtr myRAM = std::make_shared<MemoryHolder>(outputPageSet->getNewPage());

  // aggregate all hash maps
  PDBPageHandle inputPage;
  while ((inputPage = inputPageSet->getNextPage(nodeID)) != nullptr) {

    // if we haven't created an output container create it.
    if (myRAM->outputSink == nullptr) {
      myRAM->outputSink = merger->createNewOutputContainer();
    }
    // write out the page
    merger->writeOutPage(inputPage, myRAM->outputSink);

    inputPage->unpin();
  }

  // we only have one iteration
  myRAM->setIteration(0);

  // and force the reference count for this guy to go to zero
  myRAM->outputSink.emptyOutContainingBlock();

  // unpin the page so we don't have problems
  myRAM->pageHandle->unpin();
}