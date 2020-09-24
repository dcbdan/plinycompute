#pragma once

#include "Exp.h"

#include "JoinComp.h"
#include "LambdaCreationFunctions.h"

using namespace pdb;

class ExpJoin : public JoinComp<ExpJoin, Exp, Exp, Exp> {
public:
  ExpJoin() = default;

  ENABLE_DEEP_COPY

  Lambda<bool> getKeySelection(Handle<String> in1, Handle<String> in2) {
    return makeLambda(in1, in2, [](Handle<String>& in1, Handle<String>& in2){ return true; });
  }

  Lambda<Handle<String>> getKeyProjection(
    Handle<String> in1,
    Handle<String> in2)
  {
    return makeLambda(in1, in2, [](
      Handle<String> in1,
      Handle<String> in2)
    {
      return in1;
    });
  }

  Lambda<Handle<String>> getValueProjection(
    Handle<String> in1,
    Handle<String> in2)
  {
    return makeLambda(in1, in2, [](
      Handle<String> in1,
      Handle<String> in2)
    {
      return in1;
    });
  }
};

