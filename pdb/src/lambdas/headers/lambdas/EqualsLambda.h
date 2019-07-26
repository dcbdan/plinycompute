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

#ifndef EQUALS_LAM_H
#define EQUALS_LAM_H

#include <vector>
#include "Lambda.h"
#include "executors/ComputeExecutor.h"
#include "TupleSetMachine.h"
#include "TupleSet.h"
#include "Ptr.h"
#include "PDBMap.h"
#include <TypedLambdaObject.h>

namespace pdb {

// only one of these two versions is going to work... used to automatically hash on the underlying type
// in the case of a Ptr<> type
template<class MyType>
std::enable_if_t<std::is_base_of<PtrBase, MyType>::value, size_t> hashHim(MyType &him) {
  return Hasher<decltype(*him)>::hash(*him);
}

template<class MyType>
std::enable_if_t<!std::is_base_of<PtrBase, MyType>::value, size_t> hashHim(MyType &him) {
  return Hasher<MyType>::hash(him);
}

// only one of these four versions is going to work... used to automatically dereference a Ptr<blah>
// type on either the LHS or RHS of an equality check
template<class LHS, class RHS>
std::enable_if_t<std::is_base_of<PtrBase, LHS>::value && std::is_base_of<PtrBase, RHS>::value,
                 bool> checkEquals(LHS &lhs, RHS &rhs) {
  return *lhs == *rhs;
}

template<class LHS, class RHS>
std::enable_if_t<std::is_base_of<PtrBase, LHS>::value && !(std::is_base_of<PtrBase, RHS>::value),
                 bool> checkEquals(LHS &lhs, RHS &rhs) {
  return *lhs == rhs;
}

template<class LHS, class RHS>
std::enable_if_t<!(std::is_base_of<PtrBase, LHS>::value) && std::is_base_of<PtrBase, RHS>::value,
                 bool> checkEquals(LHS &lhs, RHS &rhs) {
  return lhs == *rhs;
}

template<class LHS, class RHS>
std::enable_if_t<!(std::is_base_of<PtrBase, LHS>::value) && !(std::is_base_of<PtrBase, RHS>::value), bool> checkEquals(
    LHS &lhs,
    RHS &rhs) {
  return lhs == rhs;
}

template<class LeftType, class RightType>
class EqualsLambda : public TypedLambdaObject<bool> {

public:

  EqualsLambda(LambdaTree<LeftType> lhsIn, LambdaTree<RightType> rhsIn) {
    lhs = lhsIn;
    rhs = rhsIn;
    this->setInputIndex(0, lhs.getInputIndex(0));
    this->setInputIndex(1, rhs.getInputIndex(0));
  }

  ComputeExecutorPtr getExecutor(TupleSpec &inputSchema,
                                 TupleSpec &attsToOperateOn,
                                 TupleSpec &attsToIncludeInOutput) override {

    // create the output tuple set
    TupleSetPtr output = std::make_shared<TupleSet>();

    // create the machine that is going to setup the output tuple set, using the input tuple set
    TupleSetSetupMachinePtr myMachine = std::make_shared<TupleSetSetupMachine>(inputSchema, attsToIncludeInOutput);

    // these are the input attributes that we will process
    std::vector<int> inputAtts = myMachine->match(attsToOperateOn);
    int firstAtt = inputAtts[0];
    int secondAtt = inputAtts[1];

    // this is the output attribute
    int outAtt = (int) attsToIncludeInOutput.getAtts().size();

    return std::make_shared<ApplyComputeExecutor>(
        output,
        [=](TupleSetPtr input) {

          // set up the output tuple set
          myMachine->setup(input, output);

          // get the columns to operate on
          std::vector<LeftType> &leftColumn = input->getColumn<LeftType>(firstAtt);
          std::vector<RightType> &rightColumn = input->getColumn<RightType>(secondAtt);

          // create the output attribute, if needed
          if (!output->hasColumn(outAtt)) {
            output->addColumn(outAtt, new std::vector<bool>, true);
          }

          // get the output column
          std::vector<bool> &outColumn = output->getColumn<bool>(outAtt);

          // loop down the columns, setting the output
          auto numTuples = leftColumn.size();
          outColumn.resize(numTuples);
          for (int i = 0; i < numTuples; i++) {
            outColumn[i] = checkEquals(leftColumn[i], rightColumn[i]);
          }
          return output;
        }
    );
  }

  ComputeExecutorPtr getRightHasher(TupleSpec &inputSchema,
                                    TupleSpec &attsToOperateOn,
                                    TupleSpec &attsToIncludeInOutput) override {

    // create the output tuple set
    TupleSetPtr output = std::make_shared<TupleSet>();

    // create the machine that is going to setup the output tuple set, using the input tuple set
    TupleSetSetupMachinePtr myMachine = std::make_shared<TupleSetSetupMachine>(inputSchema, attsToIncludeInOutput);

    // these are the input attributes that we will process
    std::vector<int> inputAtts = myMachine->match(attsToOperateOn);
    int secondAtt = inputAtts[0];

    // this is the output attribute
    int outAtt = (int) attsToIncludeInOutput.getAtts().size();

    return std::make_shared<ApplyComputeExecutor>(
        output,
        [=](TupleSetPtr input) {

          // set up the output tuple set
          myMachine->setup(input, output);

          // get the columns to operate on
          std::vector<RightType> &rightColumn = input->getColumn<RightType>(secondAtt);

          // create the output attribute, if needed
          if (!output->hasColumn(outAtt)) {
            output->addColumn(outAtt, new std::vector<size_t>, true);
          }

          // get the output column
          std::vector<size_t> &outColumn = output->getColumn<size_t>(outAtt);

          // loop down the columns, setting the output
          auto numTuples = rightColumn.size();
          outColumn.resize(numTuples);
          for (int i = 0; i < numTuples; i++) {
            outColumn[i] = hashHim(rightColumn[i]);
          }
          return output;
        }
    );
  }

  ComputeExecutorPtr getLeftHasher(TupleSpec &inputSchema,
                                   TupleSpec &attsToOperateOn,
                                   TupleSpec &attsToIncludeInOutput) override {

    // create the output tuple set
    TupleSetPtr output = std::make_shared<TupleSet>();

    // create the machine that is going to setup the output tuple set, using the input tuple set
    TupleSetSetupMachinePtr myMachine = std::make_shared<TupleSetSetupMachine>(inputSchema, attsToIncludeInOutput);

    // these are the input attributes that we will process
    std::vector<int> inputAtts = myMachine->match(attsToOperateOn);
    int firstAtt = inputAtts[0];

    // this is the output attribute
    int outAtt = (int) attsToIncludeInOutput.getAtts().size();

    return std::make_shared<ApplyComputeExecutor>(
        output,
        [=](TupleSetPtr input) {

          // set up the output tuple set
          myMachine->setup(input, output);

          // get the columns to operate on
          std::vector<LeftType> &leftColumn = input->getColumn<LeftType>(firstAtt);

          // create the output attribute, if needed
          if (!output->hasColumn(outAtt)) {
            output->addColumn(outAtt, new std::vector<size_t>, true);
          }

          // get the output column
          std::vector<size_t> &outColumn = output->getColumn<size_t>(outAtt);

          // loop down the columns, setting the output
          auto numTuples = leftColumn.size();
          outColumn.resize(numTuples);
          for (int i = 0; i < numTuples; i++) {
            outColumn[i] = hashHim(leftColumn[i]);
          }
          return output;
        }
    );
  }

  std::string getTypeOfLambda() override {
    return std::string("==");
  }

  int getNumChildren() override {
    return 2;
  }

  LambdaObjectPtr getChild(int which) override {
    if (which == 0)
      return lhs.getPtr();
    if (which == 1)
      return rhs.getPtr();
    return nullptr;
  }

  std::string toTCAPString(std::vector<std::string> &inputTupleSetNames,
                           std::vector<std::string> &inputColumnNames,
                           std::vector<std::string> &inputColumnsToApply,
                           std::vector<std::string> &childrenLambdaNames,
                           int lambdaLabel,
                           std::string computationName,
                           int computationLabel,
                           std::string &outputTupleSetName,
                           std::vector<std::string> &outputColumns,
                           std::string &outputColumnName,
                           std::string &myLambdaName,
                           MultiInputsBase *multiInputsComp = nullptr,
                           bool amIPartOfJoinPredicate = false,
                           bool amILeftChildOfEqualLambda = false,
                           bool amIRightChildOfEqualLambda = false,
                           std::string parentLambdaName = "",
                           bool isSelfJoin = false) override {
    std::string tcapString;
    myLambdaName = getTypeOfLambda() + "_" + std::to_string(lambdaLabel);
    std::string computationNameWithLabel = computationName + "_" + std::to_string(computationLabel);
    std::string inputTupleSetName;
    if (multiInputsComp == nullptr) {
      inputTupleSetName = inputTupleSetNames[0];
      outputTupleSetName =
          "equals_" + std::to_string(lambdaLabel) + "OutFor" + computationName + std::to_string(computationLabel);
      outputColumnName = "bool_" + std::to_string(lambdaLabel) + "_" + std::to_string(computationLabel);
      outputColumns.clear();
      for (const auto &inputColumnName : inputColumnNames) {
        outputColumns.push_back(inputColumnName);
      }
      outputColumns.push_back(outputColumnName);

      tcapString +=
          "\n/* Apply selection predicate on " + inputColumnsToApply[0] + " and " + inputColumnsToApply[1] + "*/\n";
      tcapString += this->getTCAPString(inputTupleSetName,
                                        inputColumnNames,
                                        inputColumnsToApply,
                                        outputTupleSetName,
                                        outputColumns,
                                        outputColumnName,
                                        "APPLY",
                                        computationNameWithLabel,
                                        myLambdaName,
                                        getInfo());

    } else {

      if (inputColumnNames[inputColumnNames.size() - 2] == inputColumnsToApply[0]) {
        if (inputColumnNames.size() == 4) {
          inputColumnNames[2] = inputColumnNames[1];
          inputColumnNames[1] = inputColumnsToApply[0];
        } else if (inputColumnNames.size() == 3) {
          inputColumnNames.push_back(inputColumnNames[2]);
          inputColumnNames[2] = inputColumnNames[0];
        } else {
          std::cout << "Error: right now we can't support such complex join selection "
                       "conditions"
                    << std::endl;
          exit(1);
        }
      }
      tcapString += "\n/* Join ( " + inputColumnNames[0];
      for (unsigned int i = 1; i < inputColumnNames.size() - 1; i++) {
        if (inputColumnNames[i] == inputColumnsToApply[0]) {
          tcapString += " ) and (";
        } else {
          tcapString += " " + inputColumnNames[i];
        }
      }

      tcapString += " ) */\n";
      outputTupleSetName = "JoinedFor_equals" + std::to_string(lambdaLabel) +
          computationName + std::to_string(computationLabel);
      std::string tupleSetNamePrefix = outputTupleSetName;
      outputColumns.clear();

      // TODO: push down projection here
      for (const auto &inputColumnName : inputColumnNames) {
        auto iter = std::find(
            inputColumnsToApply.begin(), inputColumnsToApply.end(), inputColumnName);
        if (iter == inputColumnsToApply.end()) {
          outputColumns.push_back(inputColumnName);
        }
      }
      outputColumnName = "";

      tcapString += outputTupleSetName + "(" + outputColumns[0];
      for (int i = 1; i < outputColumns.size(); i++) {
        tcapString += ", " + outputColumns[i];
      }
      tcapString += ") <= JOIN (" + inputTupleSetNames[0] + "(" + inputColumnsToApply[0] + "), ";
      tcapString += inputTupleSetNames[0] + "(" + inputColumnNames[0];
      int end1 = -1;
      for (int i = 1; i < inputColumnNames.size(); i++) {
        auto iter = std::find(
            inputColumnsToApply.begin(), inputColumnsToApply.end(), inputColumnNames[i]);
        if (iter != inputColumnsToApply.end()) {
          end1 = i;
          break;
        }
        tcapString += ", " + inputColumnNames[i];
      }
      if (end1 + 1 >= inputColumnNames.size()) {
        std::cout << "Can't generate TCAP for this query graph" << std::endl;
        exit(1);
      }
      tcapString += "), " + inputTupleSetNames[1] + "(" + inputColumnsToApply[1] + "), " +
          inputTupleSetNames[1] + "(" + inputColumnNames[end1 + 1];
      for (int i = end1 + 2; i < inputColumnNames.size(); i++) {
        auto iter = std::find(
            inputColumnsToApply.begin(), inputColumnsToApply.end(), inputColumnNames[i]);
        if (iter != inputColumnsToApply.end()) {
          break;
        }
        tcapString += ", " + inputColumnNames[i];
      }

      tcapString += "), '" + computationNameWithLabel + "')\n";

      inputTupleSetName = outputTupleSetName;
      inputColumnNames.clear();
      for (const auto &outputColumn : outputColumns) {
        inputColumnNames.push_back(outputColumn);
      }
      inputColumnsToApply.clear();
      inputColumnsToApply.push_back(multiInputsComp->getNameForIthInput(lhs.getInputIndex(0)));
      outputColumnName = "LHSExtractedFor_" + std::to_string(lambdaLabel) + "_" + std::to_string(computationLabel);
      outputColumns.push_back(outputColumnName);
      outputTupleSetName = tupleSetNamePrefix + "_WithLHSExtracted";

      // the additional info about this attribute access lambda
      std::map<std::string, std::string> info;

      tcapString += this->getTCAPString(inputTupleSetName,
                                        inputColumnNames,
                                        inputColumnsToApply,
                                        outputTupleSetName,
                                        outputColumns,
                                        outputColumnName,
                                        "APPLY",
                                        computationNameWithLabel,
                                        childrenLambdaNames[0],
                                        getChild(0)->getInfo());

      inputTupleSetName = outputTupleSetName;
      inputColumnNames.push_back(outputColumnName);
      inputColumnsToApply.clear();
      inputColumnsToApply.push_back(multiInputsComp->getNameForIthInput(rhs.getInputIndex(0)));
      outputTupleSetName = tupleSetNamePrefix + "_WithBOTHExtracted";
      outputColumnName = "RHSExtractedFor_" + std::to_string(lambdaLabel) + "_" + std::to_string(computationLabel);
      outputColumns.push_back(outputColumnName);

      // add the tcap string
      tcapString += this->getTCAPString(inputTupleSetName,
                                        inputColumnNames,
                                        inputColumnsToApply,
                                        outputTupleSetName,
                                        outputColumns,
                                        outputColumnName,
                                        "APPLY",
                                        computationNameWithLabel,
                                        childrenLambdaNames[1],
                                        getChild(1)->getInfo());

      inputTupleSetName = outputTupleSetName;
      inputColumnsToApply.clear();
      inputColumnsToApply.push_back("LHSExtractedFor_" + std::to_string(lambdaLabel) + "_" +
          std::to_string(computationLabel));
      inputColumnsToApply.push_back("RHSExtractedFor_" + std::to_string(lambdaLabel) + "_" +
          std::to_string(computationLabel));
      inputColumnNames.pop_back();
      outputColumnName =
          "bool_" + std::to_string(lambdaLabel) + "_" + std::to_string(computationLabel);
      outputColumns.pop_back();
      outputColumns.pop_back();
      outputColumns.push_back(outputColumnName);
      outputTupleSetName = tupleSetNamePrefix + "_BOOL";

      tcapString += this->getTCAPString(inputTupleSetName,
                                        inputColumnNames,
                                        inputColumnsToApply,
                                        outputTupleSetName,
                                        outputColumns,
                                        outputColumnName,
                                        "APPLY",
                                        computationNameWithLabel,
                                        myLambdaName,
                                        getInfo());

      inputTupleSetName = outputTupleSetName;
      outputColumnName = "";
      outputColumns.pop_back();
      outputTupleSetName = tupleSetNamePrefix + "_FILTERED";
      tcapString += outputTupleSetName + "(" + outputColumns[0];
      for (int i = 1; i < outputColumns.size(); i++) {
        tcapString += ", " + outputColumns[i];
      }
      tcapString += ") <= FILTER (" + inputTupleSetName + "(bool_" +
          std::to_string(lambdaLabel) + "_" + std::to_string(computationLabel) + "), " +
          inputTupleSetName + "(" + outputColumns[0];
      for (int i = 1; i < outputColumns.size(); i++) {
        tcapString += ", " + outputColumns[i];
      }
      tcapString += "), '" + computationNameWithLabel + "')\n";

      if (!isSelfJoin) {
        for (unsigned int index = 0; index < multiInputsComp->getNumInputs(); index++) {
          std::string curInput = multiInputsComp->getNameForIthInput(index);
          auto iter = std::find(outputColumns.begin(), outputColumns.end(), curInput);
          if (iter != outputColumns.end()) {
            multiInputsComp->setTupleSetNameForIthInput(index, outputTupleSetName);
            multiInputsComp->setInputColumnsForIthInput(index, outputColumns);
            multiInputsComp->setInputColumnsToApplyForIthInput(index, outputColumnName);
          }
        }
      }
    }
    return tcapString;
  }
  unsigned int getNumInputs() override {
    return 2;
  }
 private:

  /**
* Returns the additional information about this lambda currently just the lambda type
* @return the map
*/
  std::map<std::string, std::string> getInfo() override {
    // fill in the info
    return std::map<std::string, std::string>{
        std::make_pair("lambdaType", getTypeOfLambda())
    };
  };

private:

  LambdaTree<LeftType> lhs;
  LambdaTree<RightType> rhs;
};

}

#endif
