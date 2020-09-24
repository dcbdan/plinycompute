#pragma once

#include "TraTensor.h"

#include "LambdaCreationFunctions.h"
#include "SelectionComp.h"

using namespace pdb;

class TraFilter: public SelectionComp<TraTensor, TraTensor> {
public:
  ENABLE_DEEP_COPY

  TraFilter() = default;

  TraFilter(Vector<String> keys1, Vector<String> keys2)
    : keys1(keys1), keys2(keys2)
  {
    if(keys1.size() != keys2.size()) {
      throw std::runtime_error("TraFilter: keys must have same length");
    }
  }

  // return true only if in.meta[key1] == in.meta[key2]
  //        for every (key1, key2) in (keys1, keys2)
  //        and false otherwise
  Lambda<bool> getSelection(Handle<TraTensor> in) override {
    return makeLambda(in, [this](Handle<TraTensor>& in){
        for(int i = 0; i != keys1.size(); ++i) {
          if(in->meta->access(this->keys1[i]) != in->meta->access(this->keys2[i])) {
            return false;
          }
        }
        return true;
      });
  }

  // the keys and values are not modified
  Lambda<Handle<TraTensor>> getProjection(Handle<TraTensor> in) override {
    return makeLambda(in, [this](Handle<TraTensor>& in) {
        return in;
      });
  }

private:
  Vector<String> keys1, keys2;
};
