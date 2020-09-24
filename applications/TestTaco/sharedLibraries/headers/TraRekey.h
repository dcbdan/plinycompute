#pragma once

#include "TraTensor.h"

#include "LambdaCreationFunctions.h"
#include "SelectionComp.h"

#include "Decode.h" // for the constants

using namespace pdb;

// This can't go in decodeTra since that would be
// a circular dependency
void applyKeyOp(int*& ptr, TraMeta& meta) {
  int numOp = *ptr++;
  for(int i = 0; i != numOp; ++i) {
    if(*ptr == iKeyOpRelabel) {
      ptr++;
      std::string kin  = decodeStr(ptr);
      std::string knew = decodeStr(ptr);
      meta.relabel(kin, knew);
    }
    else if(*ptr == iKeyOpDrop) {
      ptr++;
      std::string k = decodeStr(ptr);
      meta.drop(k);
    } else {
      throw std::runtime_error("applyKeyOp");
    }
  }
}

class TraRekey: public SelectionComp<TraTensor, TraTensor> {
public:
  ENABLE_DEEP_COPY

  TraRekey() = default;

  TraRekey(Vector<int> keyop): keyop(keyop)
  {}

  Lambda<bool> getSelection(Handle<TraTensor> in) override {
    return makeLambda(in, [this](Handle<TraTensor>& in){
        return true;
      });
  }

  // modify keys to the new value
  Lambda<Handle<TraTensor>> getProjection(Handle<TraTensor> in) override {
    return makeLambda(in, [this](Handle<TraTensor>& in) {
        int* keyopPtr = keyop.c_ptr();

        // take the key by reference
        applyKeyOp(keyopPtr, in->getKeyRef());

        return in;
      });
  }

private:
  Vector<int> keyop;
};
