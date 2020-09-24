#pragma once

#include <Object.h>
#include <PDBVector.h>
#include <PDBString.h>
#include <Nothing.h>

using namespace pdb;

class Exp : public Object {
public:
  Exp() = default;
  ENABLE_DEEP_COPY
  Exp(std::string key_, std::string value_)
  {
    key = makeObject<String>(key_);
    value = makeObject<String>(value_);
  }

  Handle<String>& getKey() {
    return key;
  }

  String& getKeyRef() {
    return *key;
  }

  Handle<String>& getValue() {
    return value;
  }

  String& getValueRef() {
    return *value;
  }

  Handle<String> key;
  Handle<String> value;
};
