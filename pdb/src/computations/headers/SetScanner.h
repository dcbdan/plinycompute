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
#pragma once

#include <sources/MapTupleSetIterator.h>
#include "PDBAbstractPageSet.h"
#include "VectorTupleSetIterator.h"
#include "SourceSetArg.h"
#include "Computation.h"
#include "PDBAggregationResultTest.h"

namespace pdb {

template<class OutputClass>
class SetScanner : public Computation {
 public:

  SetScanner() = default;

  SetScanner(const std::string &db, const std::string &set) : dbName(db), setName(set) {}

  std::string getComputationType() override {
    return std::string("SetScanner");
  }

  // gets the name of the i^th input type...
  std::string getIthInputType(int i) override {
    return "";
  }

  // get the number of inputs to this query type
  int getNumInputs() override {
    return 0;
  }

  // gets the output type of this query as a string
  std::string getOutputType() override {
    return getTypeName<OutputClass>();
  }

  bool needsMaterializeOutput() override {
    return false;
  }

  // below function implements the interface for parsing computation into a TCAP string
  std::string toTCAPString(std::vector<InputTupleSetSpecifier> inputTupleSets,
                           int computationLabel,
                           std::string &outputTupleSetName,
                           std::vector<std::string> &outputColumnNames,
                           std::string &addedOutputColumnName) override {

    InputTupleSetSpecifier inputTupleSet;
    if (!inputTupleSets.empty()) {
      inputTupleSet = inputTupleSets[0];
    }
    return toTCAPString(inputTupleSet.getTupleSetName(),
                        inputTupleSet.getColumnNamesToKeep(),
                        inputTupleSet.getColumnNamesToApply(),
                        computationLabel,
                        outputTupleSetName,
                        outputColumnNames,
                        addedOutputColumnName);
  }

  /**
   * Below function returns a TCAP string for this Computation
   * @param inputTupleSetName
   * @param inputColumnNames
   * @param inputColumnsToApply
   * @param computationLabel
   * @param outputTupleSetName
   * @param outputColumnNames
   * @param addedOutputColumnName
   * @return
   */
  std::string toTCAPString(std::string inputTupleSetName,
                           std::vector<std::string> &inputColumnNames,
                           std::vector<std::string> &inputColumnsToApply,
                           int computationLabel,
                           std::string &outputTupleSetName,
                           std::vector<std::string> &outputColumnNames,
                           std::string &addedOutputColumnName) {

    // the template we are going to use to create the TCAP string for this ScanUserSet
    mustache::mustache scanSetTemplate{"inputDataFor{{computationType}}_{{computationLabel}}(in{{computationLabel}})"
                                       " <= SCAN ('{{dbName}}', '{{setName}}', '{{computationType}}_{{computationLabel}}')\n"};

    // the data required to fill in the template
    mustache::data scanSetData;
    scanSetData.set("computationType", getComputationType());
    scanSetData.set("computationLabel", std::to_string(computationLabel));
    scanSetData.set("setName", std::string(setName));
    scanSetData.set("dbName", std::string(dbName));

    // output column name
    mustache::mustache outputColumnNameTemplate{"in{{computationLabel}}"};

    //  set the output column name
    addedOutputColumnName = outputColumnNameTemplate.render(scanSetData);
    outputColumnNames.push_back(addedOutputColumnName);

    // output tuple set name template
    mustache::mustache outputTupleSetTemplate{"inputDataFor{{computationType}}_{{computationLabel}}"};
    outputTupleSetName = outputTupleSetTemplate.render(scanSetData);

    // update the state of the computation
    this->setTraversed(true);
    this->setOutputTupleSetName(outputTupleSetName);
    this->setOutputColumnToApply(addedOutputColumnName);

    // return the TCAP string
    return scanSetTemplate.render(scanSetData);
  }

  /**
   * Returns the approprate source by returning the result of a proxy method @see _getComputeSource
   * @param pageSet - the page set we are scanning
   * @param chunkSize - the size of the chunk each tuple set is going to be
   * @param workerID - the id of the worker
   * @param params - the pipeline parameters we use to get the info about the set
   * @return can be either @see VectorTupleSetIterator or @see MapTupleSetIterator depending on the type of the set
   */
  pdb::ComputeSourcePtr getComputeSource(const PDBAbstractPageSetPtr &pageSet,
                                         size_t chunkSize,
                                         uint64_t workerID,
                                         std::map<ComputeInfoType, ComputeInfoPtr> &params) override {

    return _getComputeSource(pageSet, chunkSize, workerID, params);
  }

 private:

  /**
   * This is the method that is going to be instantiate if the OutputClass can be a result of an aggregation.
   * This means that it has getValue and getKey methods defined.
   * If checks the type of the set to see if it is a vector set or a set that is a result of an aggregation.
   * based on that it returns the approprate source
   * @tparam T - alias for the output type
   * @param pageSet - the page set we are scanning
   * @param chunkSize - the size of the chunk each tuple set is going to be
   * @param workerID - the id of the worker
   * @param params - the pipeline parameters we use to get the info about the set
   * @return can be either @see VectorTupleSetIterator or @see MapTupleSetIterator depending on the type of the set
   */
  template<class T = OutputClass>
  typename std::enable_if_t<hasGetKey<T>::value and hasGetValue<T>::value, pdb::ComputeSourcePtr>
  _getComputeSource(const PDBAbstractPageSetPtr &pageSet,
                                          size_t chunkSize,
                                          uint64_t workerID,
                                          std::map<ComputeInfoType, ComputeInfoPtr> &params) {

    // declare upfront the key and the value types
    using Value = typename std::remove_reference<decltype(std::declval<T>().getValue())>::type;
    using Key = typename std::remove_reference<decltype(std::declval<T>().getKey())>::type;

    auto sourceSetInfo = std::dynamic_pointer_cast<SourceSetArg>(params[ComputeInfoType::SOURCE_SET_INFO]);

    // check if we actually have it
    if(sourceSetInfo == nullptr && sourceSetInfo->set == nullptr) {
      throw runtime_error("Did not get any info about set ("  + (string) dbName +"," + (string) setName +")");
    }

    if(sourceSetInfo->set->containerType == PDB_CATALOG_SET_VECTOR_CONTAINER) {
      return std::make_shared<pdb::VectorTupleSetIterator>(pageSet, chunkSize, workerID);
    }
    else if(sourceSetInfo->set->containerType == PDB_CATALOG_SET_MAP_CONTAINER) {
      return std::make_shared<pdb::MapTupleSetIterator<Key, Value, OutputClass>> (pageSet, workerID, chunkSize);
    }

    throw runtime_error("Unknown container  type for set ("  + (string) dbName +"," + (string) setName +")");
  }

  /**
   * This is the method that is going to be instantiate if the OutputClass does not even qualify to be a result of an
   * aggregation.
   * @tparam T - alias for the output type
   * @param pageSet - the page set we are scanning
   * @param chunkSize - the size of the chunk each tuple set is going to be
   * @param workerID - the id of the worker
   * @param params - the pipeline parameters we use to get the info about the set
   * @return the appropriate compute source, currently this can only be an @see VectorTupleSetIterator
   */
  template<class T = OutputClass>
  typename std::enable_if_t<!hasGetKey<T>::value or !hasGetValue<T>::value, pdb::ComputeSourcePtr>
   _getComputeSource(const PDBAbstractPageSetPtr &pageSet,
                                          size_t chunkSize,
                                          uint64_t workerID,
                                          std::map<ComputeInfoType, ComputeInfoPtr> &params) {
    auto sourceSetInfo = std::dynamic_pointer_cast<SourceSetArg>(params[ComputeInfoType::SOURCE_SET_INFO]);

    // check if we actually have it
    if(sourceSetInfo == nullptr && sourceSetInfo->set == nullptr) {
      throw runtime_error("Did not get any info about set ("  + (string) dbName +"," + (string) setName +")");
    }

    if(sourceSetInfo->set->containerType == PDB_CATALOG_SET_VECTOR_CONTAINER) {
      return std::make_shared<pdb::VectorTupleSetIterator>(pageSet, chunkSize, workerID);
    }

    // this is not good
    throw runtime_error("Unknown container  type for set ("  + (string) dbName +"," + (string) setName +")");
  }

  /**
   * The name of the database the set we are scanning belongs to
   */
  pdb::String dbName;

  /**
   * The name of the set we are scanning
   */
  pdb::String setName;

};

}