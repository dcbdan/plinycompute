#pragma once

#include "TraTensor.h"

#include "LambdaCreationFunctions.h"
#include "AggregateComp.h"

using namespace pdb;

//                                 Derived  Out        Input      Key      Value
class TraAgg : public AggregateComp<TraAgg, TraTensor, TraTensor, TraMeta, TacoTensor> {
public:
  ENABLE_DEEP_COPY

  TraAgg() = default;

  TraAgg(Vector<String> whichToAgg): whichToAgg(whichToAgg)
  {}

  Lambda<TraMeta> getKeyProjectionWithInputKey(Handle<TraMeta> aggMe) {
    return makeLambda(aggMe, [this](Handle<TraMeta>& in) {
        // remove whichToAgg keys
        auto& infoIn = in->info;
        size_t size = infoIn.size() - this->whichToAgg.size();
        Vector<StringIntPair> infoOut(size, size);
        int which = 0;
        for(int i = 0; i != infoIn.size(); ++i) {
          // remove whichToAgg from infoIn
          if(!this->isToAgg(*(infoIn[i].myString))) {
            infoOut[which++] = infoIn[i];
          }
        }

        return TraMeta(infoOut);
        //Handle<TraMeta> out = makeObject<TraMeta>(infoOut);
        //return out;
      });
  }

  Lambda<TacoTensor> getValueProjection(Handle<TraTensor> aggMe) {
    return makeLambdaFromMethod(aggMe, getValueRef);
  }

private:
  Vector<String> whichToAgg;

  bool isToAgg(String& s) {
    for(int i = 0; i != whichToAgg.size(); ++i) {
      if(s == whichToAgg[i]) {
        return true;
      }
    }
    return false;
  }
};
