#ifndef STRING_STRING_PAIR_H
#define STRING_STRING_PAIR_H

#include "Object.h"
#include "PDBString.h"
#include "Handle.h"

namespace pdb {

class StringStringPair : public Object {

 public:

  Handle<String> myKey;
  Handle<String> myValue;

  Handle<String>& getKey(){
    return myKey;
  }
  Handle<String>& getValue(){
    return myValue;
  }

  ENABLE_DEEP_COPY

  ~StringStringPair() {}
  StringStringPair() {}

  StringStringPair(std::string fromMeKey, std::string fromMeValue) {
    myKey   = makeObject<String>(fromMeKey);
    myValue = makeObject<String>(fromMeValue);
  }
};

}

#endif
