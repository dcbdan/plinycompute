#pragma once

#include <LambdaCreationFunctions.h>
#include "StringStringPair.h"
#include "JoinComp.h"

namespace pdb {

class SillyCartasianJoinStringString : public JoinComp <SillyCartasianJoinStringString, StringStringPair, StringStringPair, StringStringPair> {
public:

  ENABLE_DEEP_COPY

  SillyCartasianJoinStringString() = default;

  static Lambda<bool> getKeySelection(Handle<String> in1, Handle<String> in2) {
    return makeLambda(in1, in2, [](Handle<String>&, Handle<String>&){ return true; });
  }

  static Lambda<Handle<String> > getKeyProjection(Handle<String> in1, Handle<String> in2) {
    return concatenate(in1, in2);
  }

  static Lambda<Handle<String> > getValueProjection(Handle<String> in1, Handle<String> in2) {
    return concatenate(in1, in2);
  }

private:
  static Lambda<Handle<String> > concatenate(Handle<String>& in1, Handle<String>& in2) {
    return makeLambda(in1, in2, [](Handle<String>& in1, Handle<String>& in2) {
        std::string left(*in1);
        std::string right(*in2);
        Handle<String> out = makeObject<String>(left + right);
        return out;
    });
  }
};

}
