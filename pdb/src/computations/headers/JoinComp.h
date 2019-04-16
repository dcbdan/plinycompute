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

#ifndef JOIN_COMP
#define JOIN_COMP

#include <JoinTupleSingleton.h>
#include "Computation.h"
#include "JoinTests.h"
#include "ComputePlan.h"
#include "JoinTuple.h"
#include "JoinCompBase.h"
#include "LogicalPlan.h"
#include "ShuffleJoinSide.h"

namespace pdb {

template<typename Out, typename In1, typename In2, typename ...Rest>
class JoinComp : public JoinCompBase {

 private:

  std::vector<std::string> tupleSetNamesForInputs;

 public:

  // the computation returned by this method is called to see if a data item should be returned in the output set
  virtual Lambda<bool> getSelection(Handle<In1> in1, Handle<In2> in2, Handle<Rest> ...otherArgs) = 0;

  // the computation returned by this method is called to produce output tuples from this method
  virtual Lambda<Handle<Out>> getProjection(Handle<In1> in1, Handle<In2> in2, Handle<Rest> ...otherArgs) = 0;

  // calls getProjection and getSelection to extract the lambdas
  void extractLambdas(std::map<std::string, LambdaObjectPtr> &returnVal) override {
    int suffix = 0;
    Lambda<bool> selectionLambda = callGetSelection(*this);
    Lambda<Handle<Out>> projectionLambda = callGetProjection(*this);
    selectionLambda.toMap(returnVal, suffix);
    projectionLambda.toMap(returnVal, suffix);
  }

  // return the output type
  std::string getOutputType() override {
    return getTypeName<Out>();
  }

  // count the number of inputs
  int getNumInputs() final {
    const int extras = sizeof...(Rest);
    return extras + 2;
  }

  template<typename First, typename ...Others>
  typename std::enable_if<sizeof ...(Others) == 0, std::string>::type getIthInputType(int i) {
    if (i == 0) {
      return getTypeName<First>();
    } else {
      std::cout << "Asked for an input type that didn't exist!";
      exit(1);
    }
  }

  // helper function to get a particular intput type
  template<typename First, typename ...Others>
  typename std::enable_if<sizeof ...(Others) != 0, std::string>::type getIthInputType(int i) {
    if (i == 0) {
      return getTypeName<First>();
    } else {
      return getIthInputType<Others...>(i - 1);
    }
  }

  // from the interface: get the i^th input type
  std::string getIthInputType(int i) final {
    return getIthInputType<In1, In2, Rest...>(i);
  }

  // this gets a compute sink
  ComputeSinkPtr getComputeSink(TupleSpec &consumeMe,
                                TupleSpec &attsToOpOn,
                                TupleSpec &projection,
                                uint64_t numPartitions,
                                std::map<ComputeInfoType, ComputeInfoPtr> &params,
                                pdb::LogicalPlanPtr &plan) override {

    // loop through each of the attributes that we are supposed to accept, and for each of them, find the type
    std::vector<std::string> typeList;
    AtomicComputationPtr producer = plan->getComputations().getProducingAtomicComputation(consumeMe.getSetName());
    std::cout << "consumeMe was: " << consumeMe << "\n";
    std::cout << "attsToOpOn was: " << attsToOpOn << "\n";
    std::cout << "projection was: " << projection << "\n";
    for (auto &a : projection.getAtts()) {

      // find the identity of the producing computation
      std::cout << "finding the source of " << projection.getSetName() << "." << a << "\n";
      std::pair<std::string, std::string> res = producer->findSource(a, plan->getComputations());
      std::cout << "got " << res.first << " " << res.second << "\n";

      // and find its type... in the first case, there is not a particular lambda that we need to ask for
      if (res.second.empty()) {
        typeList.push_back("pdb::Handle<" + plan->getNode(res.first).getComputation().getOutputType() + ">");
      } else {
        typeList.push_back("pdb::Handle<" + plan->getNode(res.first).getLambda(res.second)->getOutputType() + ">");
      }
    }

    for (auto &aa : typeList) {
      std::cout << "Got type " << aa << "\n";
    }

    // now we get the correct join tuple, that will allow us to pack tuples from the join in a hash table
    std::vector<int> whereEveryoneGoes;
    JoinTuplePtr correctJoinTuple = findCorrectJoinTuple<In1, In2, Rest...>(typeList, whereEveryoneGoes);

    for (auto &aa : whereEveryoneGoes) {
      std::cout << aa << " ";
    }
    std::cout << "\n";

    // check
    auto it = params.find(ComputeInfoType::JOIN_SIDE);
    ShuffleJoinSidePtr joinSide = dynamic_pointer_cast<ShuffleJoinSide>(it->second);

    if(joinSide->value == ShuffleJoinSideEnum::PROBE_SIDE){

      return correctJoinTuple->getProbeSink(consumeMe, attsToOpOn, projection, whereEveryoneGoes, numPartitions);
    }
    else {
      return correctJoinTuple->getBuildSink(consumeMe, attsToOpOn, projection, whereEveryoneGoes, numPartitions);
    }
  }

  // this gets a merger sink
  ComputeSinkPtr getComputeMerger(TupleSpec &consumeMe, TupleSpec &attsToOpOn, TupleSpec &projection,
                                  uint64_t workerID, uint64_t numPartitions, pdb::LogicalPlanPtr &plan) override {

    // loop through each of the attributes that we are supposed to accept, and for each of them, find the type
    std::vector<std::string> typeList;
    AtomicComputationPtr producer = plan->getComputations().getProducingAtomicComputation(consumeMe.getSetName());
    std::cout << "consumeMe was: " << consumeMe << "\n";
    std::cout << "attsToOpOn was: " << attsToOpOn << "\n";
    std::cout << "projection was: " << projection << "\n";
    for (auto &a : projection.getAtts()) {

      // find the identity of the producing computation
      std::cout << "finding the source of " << projection.getSetName() << "." << a << "\n";
      std::pair<std::string, std::string> res = producer->findSource(a, plan->getComputations());
      std::cout << "got " << res.first << " " << res.second << "\n";

      // and find its type... in the first case, there is not a particular lambda that we need to ask for
      if (res.second.empty()) {
        typeList.push_back("pdb::Handle<" + plan->getNode(res.first).getComputation().getOutputType() + ">");
      } else {
        typeList.push_back("pdb::Handle<" + plan->getNode(res.first).getLambda(res.second)->getOutputType() + ">");
      }
    }

    for (auto &aa : typeList) {
      std::cout << "Got type " << aa << "\n";
    }

    // now we get the correct join tuple, that will allow us to pack tuples from the join in a hash table
    std::vector<int> whereEveryoneGoes;
    JoinTuplePtr correctJoinTuple = findCorrectJoinTuple<In1, In2, Rest...>(typeList, whereEveryoneGoes);

    for (auto &aa : whereEveryoneGoes) {
      std::cout << aa << " ";
    }
    std::cout << "\n";

    // return the merger
    return correctJoinTuple->getMerger(workerID, numPartitions);
  }

  // this is a join computation
  std::string getComputationType() override {
    return std::string("JoinComp");
  }

  //JiaNote: Returning a TCAP string for this Join computation
  virtual std::string toTCAPString(std::vector<InputTupleSetSpecifier> inputTupleSets,
                                   int computationLabel,
                                   std::string &outputTupleSetName,
                                   std::vector<std::string> &outputColumnNames,
                                   std::string &addedOutputColumnName) override {
    return "";
  }

  // JiaNote: returns the latest tuple set name that contains the i-th input
  std::string getTupleSetNameForIthInput(int i) {
    if (i > this->getNumInputs()) {
      return "";
    }
    return tupleSetNamesForInputs[i];
  }

  // JiaNote: set the latest tuple set name that contains the i-th input
  void setTupleSetNameForIthInput(int i, std::string name) {
    int numInputs = this->getNumInputs();
    if (tupleSetNamesForInputs.size() != numInputs) {
      tupleSetNamesForInputs.resize(numInputs, 0);
    }
    if (i < numInputs) {
      tupleSetNamesForInputs[i] = name;
    }
  }

  // gets an execute that can run a scan join... needToSwapAtts is true if the atts that are currently stored in the hash table need to
  // come SECOND in the output tuple sets... hashedInputSchema tells us the schema for the attributes that are currently stored in the
  // hash table... pipelinedInputSchema tells us the schema for the attributes that will be coming through the pipeline...
  // pipelinedAttsToOperateOn is the identity of the hash attribute... pipelinedAttsToIncludeInOutput tells us the set of attributes
  // that are coming through the pipeline that we actually have to write to the output stream
  ComputeExecutorPtr getExecutor(bool needToSwapAtts,
                                 TupleSpec &hashedInputSchema,
                                 TupleSpec &pipelinedInputSchema,
                                 TupleSpec &pipelinedAttsToOperateOn,
                                 TupleSpec &pipelinedAttsToIncludeInOutput,
                                 JoinArgPtr &joinArg,
                                 uint64_t numProcessingThreads,
                                 uint64_t workerID,
                                 ComputePlan &computePlan) override {


    std::cout << "pipelinedInputSchema is " << pipelinedInputSchema << "\n";
    std::cout << "pipelinedAttsToOperateOn is " << pipelinedAttsToOperateOn << "\n";
    std::cout << "pipelinedAttsToIncludeInOutput is " << pipelinedAttsToIncludeInOutput << "\n";
    std::cout << "From the join arg, got " << joinArg->hashTablePageSet->getNumPages() << "\n";

    // loop through each of the attributes that we are supposed to accept, and for each of them, find the type
    std::vector<std::string> typeList;
    AtomicComputationPtr producer = computePlan.getPlan()->getComputations().getProducingAtomicComputation(hashedInputSchema.getSetName());
    for (auto &a : (hashedInputSchema.getAtts())) {

      // find the identity of the producing computation
      std::cout << "finding the source of " << hashedInputSchema.getSetName() << "." << a << "\n";
      std::pair<std::string, std::string> res = producer->findSource(a, computePlan.getPlan()->getComputations());

      // and find its type... in the first case, there is not a particular lambda that we need to ask for
      if (res.second.empty()) {
        typeList.push_back(
            "pdb::Handle<" + computePlan.getPlan()->getNode(res.first).getComputation().getOutputType() + ">");
      } else {
        typeList.push_back(
            "pdb::Handle<" + computePlan.getPlan()->getNode(res.first).getLambda(res.second)->getOutputType() + ">");
      }
    }

    for (auto &aa : typeList) {
      std::cout << "Got type " << aa << "\n";
    }

    // now we get the correct join tuple, that will allow us to pack tuples from the join in a hash table
    std::vector<int> whereEveryoneGoes;
    JoinTuplePtr correctJoinTuple = findCorrectJoinTuple<In1, In2, Rest...>(typeList, whereEveryoneGoes);

    std::cout << "whereEveryoneGoes was: ";
    for (auto &a : whereEveryoneGoes) {
      std::cout << a << " ";
    }
    std::cout << "\n";

    // and return the correct probing code
    return correctJoinTuple->getProber(joinArg->hashTablePageSet,
                                       whereEveryoneGoes,
                                       pipelinedInputSchema,
                                       pipelinedAttsToOperateOn,
                                       pipelinedAttsToIncludeInOutput,
                                       numProcessingThreads,
                                       workerID,
                                       needToSwapAtts);
  }

  ComputeExecutorPtr getExecutor(bool needToSwapAtts,
                                 TupleSpec &hashedInputSchema,
                                 TupleSpec &pipelinedInputSchema,
                                 TupleSpec &pipelinedAttsToOperateOn,
                                 TupleSpec &pipelinedAttsToIncludeInOutput) override {
    std::cout << "Currently, no pipelined version of the join doesn't take an arg.\n";
    exit(1);
  }

};

}

#endif
