#pragma once

#include "Decode.h"
#include "Tra.h"

// This is tedious... The decode is how I take algebraic data structures
// from haskell and put them into C++ land.
// On the haskell side, there is an encode function that takes everything
// and turns it into a big sequence of integers.
// The decode takes a big sequence of integers and reads it into
// whatever it is meant to represent.
using GetInput = std::function<
  Handle<Computation>(
    std::string const&,
    pair<Type, Type> const&)>;

Handle<Computation> decodeTra(int*& ptr, GetInput& getIn);
// All of these decode functions take as input a reference to a pointer--
// so that each decode intentionally consumes (increments) ptr. When it
// completes, it leaves the pointer ready to parse the next thing.

Handle<Computation> decodeTraTensor(int*& ptr, GetInput& getIn) {
  if(*ptr != iTensor) {
    throw std::runtime_error("TraTensor");
  }
  ptr++; // remove iTensor
  pair<Type, Type> traTensorType = decodeRel(ptr);
  std::string name = decodeStr(ptr);

  return getIn(name, traTensorType);
}

Handle<Computation> decodeTraAgg(int*& ptr, GetInput& getIn) {
  if(*ptr != iAgg) {
    throw std::runtime_error("TraAgg");
  }
  ptr++;
  decodeRel(ptr);
  Vector<String> whichToAgg;
  while(*ptr == iStr) {
    whichToAgg.push_back(decodeStr(ptr));
  }

  // (don't) get the expr
  if(*ptr != iExpr) {
    throw std::runtime_error("TraAgg expr");
  }
  ptr++;
  // TODO: as the moment, it is just assumed that this is a BinOp "+" op!
  int numToSkip = *ptr++;
  ptr += numToSkip;

  Handle<Computation> out = makeObject<TraAgg>(whichToAgg);
  out->setInput(decodeTra(ptr, getIn));

  return out;
}

Handle<Computation> decodeTraJoin(int*& ptr, GetInput& getIn) {
  if(*ptr != iJoin) {
    throw std::runtime_error("TraJoin");
  }
  ptr++;

  // rel contains ([Key:Size], [Key:Size])
  // where fst is for the whole relation and the snd
  // is for the tensors stored within
  auto rel = decodeRel(ptr);
  Type const& d = rel.second;
  Vector<int> dims;
  dims.reserve(d.size());
  for(int i = 0; i != d.size(); ++i) {
    dims[i] = d[i].second;
  }

  auto keys1keys2 = decodeKeyPairs(ptr);
  Vector<String> keys1 = keys1keys2.first;
  Vector<String> keys2 = keys1keys2.second;

  // get the expr
  if(*ptr != iExpr) {
    throw std::runtime_error("TraAgg expr");
  }
  ptr++;
  int numToCopy = *ptr++;
  Vector<int> expr(numToCopy, numToCopy);
  for(int i = 0; i != numToCopy; ++i) {
    expr[i] = *ptr++;
  } // TODO: c'mon, don't use a for loop here

  Handle<Computation> out = makeObject<TraJoin>(keys1, keys2, expr, dims);

  out->setInput(0, decodeTra(ptr, getIn));
  out->setInput(1, decodeTra(ptr, getIn));

  return out;
}

Handle<Computation> decodeTraTransform(int*& ptr, GetInput& getIn) {
  if(*ptr != iTransform) {
    throw std::runtime_error("TraTransform");
  }
  ptr++;
  auto rel = decodeRel(ptr);
  Type const& d = rel.second;
  Vector<int> dims;
  dims.reserve(d.size());
  for(int i = 0; i != d.size(); ++i) {
    dims[i] = d[i].second;
  }

  // get the expr
  if(*ptr != iExpr) {
    throw std::runtime_error("TraAgg expr");
  }
  ptr++;
  int numToCopy = *ptr++;
  Vector<int> expr(numToCopy, numToCopy);
  for(int i = 0; i != numToCopy; ++i) {
    expr[i] = *ptr++;
  } // TODO: c'mon, don't use a for loop here

  Handle<Computation> out = makeObject<TraTransform>(expr, dims);

  out->setInput(decodeTra(ptr, getIn));

  return out;
}

Handle<Computation> decodeTraRekey(int*& ptr, GetInput& getIn) {
  if(*ptr != iRekey) {
    throw std::runtime_error("TraRekey");
  }
  ptr++;
  decodeRel(ptr);

  // get the keyop
  if(*ptr != iKeyOp) {
    throw std::runtime_error("TraRekey keyop");
  }
  ptr++;
  int numToCopy = *ptr++;
  Vector<int> keyop(numToCopy, numToCopy);
  for(int i = 0; i != numToCopy; ++i) {
    keyop[i] = *ptr++;
  } // TODO: c'mon, don't use a for loop here

  Handle<Computation> out = makeObject<TraRekey>(keyop);

  out->setInput(decodeTra(ptr, getIn));

  return out;
}

Handle<Computation> decodeTraFilterEqKey(int*& ptr, GetInput& getIn) {
  if(*ptr != iFilterEqKey) {
    throw std::runtime_error("TraFilterEqKey");
  }
  ptr++;
  decodeRel(ptr);

  auto keys1keys2 = decodeKeyPairs(ptr);
  Vector<String> keys1 = keys1keys2.first;
  Vector<String> keys2 = keys1keys2.second;

  Handle<Computation> out = makeObject<TraFilter>(keys1, keys2);

  out->setInput(decodeTra(ptr, getIn));

  return out;
}

Handle<Computation> decodeTra(int*& ptr, GetInput& getIn) {
  if(*ptr == iTensor) {
    std::cout << "TENSOR" << std::endl;
    return decodeTraTensor(ptr, getIn);
  }
  if(*ptr == iAgg) {
    std::cout << "AGG" << std::endl;
    return decodeTraAgg(ptr, getIn);
  }
  if(*ptr == iJoin) {
    std::cout << "JOIN" << std::endl;
    return decodeTraJoin(ptr, getIn);
  }
  if(*ptr == iTransform) {
    std::cout << "TRANSFORM" << std::endl;
    return decodeTraTransform(ptr, getIn);
  }
  if(*ptr == iRekey) {
    std::cout << "REKEY" << std::endl;
    return decodeTraRekey(ptr, getIn);
  }
  if(*ptr == iFilterEqKey) {
    std::cout << "FILTEREQKEY" << std::endl;
    return decodeTraFilterEqKey(ptr, getIn);
  }

  throw std::runtime_error("decodeTra");
}



