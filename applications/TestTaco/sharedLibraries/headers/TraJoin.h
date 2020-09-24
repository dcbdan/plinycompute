#pragma once

#include "TraTensor.h"

#include "JoinComp.h"
#include "LambdaCreationFunctions.h"

#include "Decode.h"

using namespace pdb;

//                              Derived, Out,       In1,       In2
class TraJoin : public JoinComp<TraJoin, TraTensor, TraTensor, TraTensor> {
public:
  ENABLE_DEEP_COPY

  TraJoin() = default;

  TraJoin(
    Vector<String> keys1, Vector<String> keys2,
    Vector<int> expr,
    Vector<int> tensorDimension)
    : expr(expr), keys1(keys1), keys2(keys2), tensorDimension(tensorDimension)
  {
    if(keys1.size() != keys2.size()) {
      throw std::runtime_error("TraJoin: Key sizes must be the same");
    }
  }

  // Return true if the index for each of the matching keys is correct.
  // [i: 5, j:3], [j:5] -> returns false on (key1, key2) = [j,j]
  //                    -> returns true on  (key1, key2) = [i,j]
  Lambda<bool> getKeySelection(Handle<TraMeta> in1, Handle<TraMeta> in2) {
    return makeLambda(in1, in2, [this](Handle<TraMeta>& in1, Handle<TraMeta>& in2) {
        for(int i = 0; i != keys1.size(); ++i) {
          if(in1->access(keys1[i]) != in2->access(keys2[i])) {
            return false;
          }
        }
        return true;
      });
  }

  Lambda<Handle<TraMeta>>
  getKeyProjection(
    Handle<TraMeta> in1,
    Handle<TraMeta> in2)
  {
    return makeLambda(in1, in2, [this](
      Handle<TraMeta>& in1,
      Handle<TraMeta>& in2)
    {
      auto size = in1->size() + in2->size() - keys1.size();
      Vector<StringIntPair> info(size, size);

      int whichNext = 0;
      auto addToInfo = [&](std::string& s, int& i) {
        info[whichNext++] = StringIntPair(String(s), i);
      };

      auto isNotInKeys2 = [&, this](String& s) {
        for(int j = 0; j != keys2.size(); ++j) {
          if(s == keys2[j])
            return false;
        }
        return true;
      };

      std::string s;
      for(int i = 0; i != in1->size(); ++i) {
        s = "&" + std::string(*(in1->info[i].myString));
        addToInfo(s, in1->info[i].myInt);
      }

      for(int i = 0; i != in2->size(); ++i) {
        // only add it if in2.info[i].myString is not in keys2
        if(isNotInKeys2(*(in2->info[i].myString))){
          s = "*" + std::string(*(in2->info[i].myString));
          addToInfo(s, in2->info[i].myInt);
        }
      }

      Handle<TraMeta> out = makeObject<TraMeta>(info);
      return out;
    });
  }

  Lambda<Handle<TacoTensor>>
  getValueProjection(
      Handle<TacoTensor> in1,
      Handle<TacoTensor> in2)
  {
    return makeLambda(in1, in2, [this](
        Handle<TacoTensor>& in1,
        Handle<TacoTensor>& in2)
    {
      // this knows the output dimension, just put it into a std::vector
      std::vector<int> dimensions = this->getOutputDimension();

      // At the moment, all tensors are assumed to be dense
      // and doubles. TODO: it would be nice to dynamically choose
      // the output shape
      taco::Format outputFormat(
        std::vector<taco::ModeFormatPack>(dimensions.size(), taco::dense));

      // this gets all the taco meta setup, included that allocation,
      // but it does not allocate the actual tensor, which is done in
      // callKernel
      Handle<TacoTensor> out = makeObject<TacoTensor>(
          taco::type<double>(),
          dimensions,
          outputFormat);

      // build the taco assignment statement from expr
      int* asgPtr = this->expr.c_ptr();
      taco::Assignment assignment = decodeAssignment(asgPtr);

      // - run the computation, this will allocate
      //   memory in tacoTensors[0] as necessary.
      // - it should also work regardless of the
      //   sparsity structure of each of the tacoTensors
      // - it should also work for whatever the underlying
      //   computation contained in assignment is
      std::vector<Handle<TacoTensor>> tacoTensors = {out, in1, in2};
      TacoTensor::callKernel(assignment, tacoTensors);

      return out;
    });
  }

private:
  Vector<String> keys1, keys2;
  Vector<int> expr;
  Vector<int> tensorDimension;

  std::vector<int> getOutputDimension() {
    std::vector<int> out;
    out.reserve(tensorDimension.size());
    for(int i = 0; i != tensorDimension.size(); ++i) {
      out[i] = tensorDimension[i];
    }
    return out;
  }
};

