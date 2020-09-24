#pragma once

#include "TraTensor.h"

#include "LambdaCreationFunctions.h"
#include "SelectionComp.h"

#include "Decode.h"

using namespace pdb;

class TraTransform: public SelectionComp<TraTensor, TraTensor> {
public:
  ENABLE_DEEP_COPY

  TraTransform() = default;

  TraTransform(Vector<int> expr, Vector<int> tensorDimension)
    : expr(expr), tensorDimension(tensorDimension)
  {}

  Lambda<bool> getSelection(Handle<TraTensor> in) override {
    return makeLambda(in, [](Handle<TraTensor>& in){ return true; });
  }

  Lambda<Handle<TraTensor>> getProjection(Handle<TraTensor> in) override {
    return makeLambda(in, [this](Handle<TraTensor>& in) {
      // see TraJoin's getValueProjection for more detailed comments
      // .. It's the same thing.

      std::vector<int> dimensions = this->getOutputDimension();

      taco::Format outputFormat(
        std::vector<taco::ModeFormatPack>(dimensions.size(), taco::dense));

      Handle<TacoTensor> out = makeObject<TacoTensor>(
          taco::type<double>(),
          dimensions,
          outputFormat);

      int* asgPtr = this->expr.c_ptr();
      taco::Assignment assignment = decodeAssignment(asgPtr);

      std::vector<Handle<TacoTensor>> tacoTensors = {out, in->getValue()};
      TacoTensor::callKernel(assignment, tacoTensors);

      Handle<TraTensor> outKeyValue = makeObject<TraTensor>(out, in->getKey());
      return outKeyValue;
    });
  }

private:
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

