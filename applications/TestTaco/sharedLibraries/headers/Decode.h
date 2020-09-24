#pragma once

#include <taco/format.h>
#include <taco/type.h>

int const iTensor = 1;
int const iAgg = 2;
int const iJoin = 3;
int const iTransform = 4;
int const iRekey = 5;
int const iFilterEqKey = 6;
int const iStr = 7;
int const iRel = 8;
int const iType = 9;
int const iExpr = 10;
int const iKeyOp = 11;
int const iTAssignment = 12;
int const iTPlus = 13;
int const iTMult = 14;
int const iTTensor = 15;
int const iKeyOpDrop = 16;
int const iKeyOpRelabel = 17;
int const iKeyOpId = 18;

using Type = vector<pair<string, int>>;

taco::TensorVar createDenseTensorVar(int numModes) {
  std::vector<taco::ModeFormatPack> modeFormat(numModes, taco::dense);
  std::vector<taco::Dimension> dynamicDimensions(numModes);
  taco::Type theType(taco::type<double>(), dynamicDimensions);
  return taco::TensorVar(theType, taco::Format(modeFormat));
}

string decodeStr(int*& ptr) {
  if(*ptr != iStr) {
    throw std::runtime_error("decodeStr");
  }

  ptr++; // remove iStr
  int len = *ptr++;
  vector<char> theStrVec;
  for(int cnt = 0; cnt != len; ++cnt) {
    theStrVec.push_back(*ptr++);
  }

  return std::string(theStrVec.begin(), theStrVec.end());
}

Type decodeType(int*& ptr) {
  if(*ptr != iType) {
    throw std::runtime_error("decodeType");
  }

  ptr++; // remove iType
  int numToGet = *ptr++;

  std::vector<std::pair<std::string, int> > ret;
  ret.reserve(numToGet);
  for(int cnt = 0; cnt != numToGet; ++cnt) {
    auto s = decodeStr(ptr);
    ret.push_back(make_pair(s, *ptr++));
  }

  return ret;
}

pair<Type, Type> decodeRel(int*& ptr) {
  if(*ptr != iRel) {
    throw std::runtime_error("decodeRel");
  }

  ptr++; // remove iRel
  auto T1 = decodeType(ptr);
  auto T2 = decodeType(ptr);
  return make_pair(T1, T2);
}

std::pair<Vector<String>, Vector<String> >
decodeKeyPairs(int*& ptr) {
  int numToGet = *ptr++;
  Vector<String> ret1;
  Vector<String> ret2;

  for(int cnt = 0; cnt != numToGet; ++cnt) {
    ret1.push_back(decodeStr(ptr));
    ret2.push_back(decodeStr(ptr));
  }

  return std::make_pair(ret1, ret2);
}

// forward declarations
taco::Assignment decodeAssignment(int*& ptr);
taco::IndexExpr  decodeTExpr(int*& ptr);

taco::IndexExpr decodeTPlus(int*& ptr) {
  if(*ptr != iTPlus) {
    throw std::runtime_error("decodeTPlus");
  }
  ptr++;

  auto left = decodeTExpr(ptr);
  auto right = decodeTExpr(ptr);
  return (left + right);
}

taco::IndexExpr decodeTMult(int*& ptr) {
  if(*ptr != iTMult) {
    throw std::runtime_error("decodeTMult");
  }
  ptr++;

  auto left = decodeTExpr(ptr);
  auto right = decodeTExpr(ptr);
  return (left * right);
}

taco::Access decodeAccess(int*& ptr) {
  int numKeys = *ptr++;
  std::vector<taco::IndexVar> keys;
  keys.reserve(numKeys);
  for(int i = 0; i != numKeys; ++i) {
    std::string name = decodeStr(ptr);
    keys.emplace_back(name);
  }

  // out tensorVar is just a dense
  taco::TensorVar var = createDenseTensorVar(numKeys);
  return var(keys);
}

taco::IndexExpr decodeTTensor(int*& ptr) {
  if(*ptr != iTTensor) {
    throw std::runtime_error("decodeTTensor");
  }
  ptr++;

  return decodeAccess(ptr);
}

taco::IndexExpr decodeTExpr(int*& ptr) {
  if(*ptr == iTPlus) {
    return decodeTPlus(ptr);
  }
  if(*ptr == iTMult) {
    return decodeTMult(ptr);
  }
  if(*ptr == iTTensor) {
    return decodeTTensor(ptr);
  }

  throw std::runtime_error("decodeTExpr");
  return taco::IndexExpr();
}

taco::Assignment decodeAssignment(int*& ptr) {
  if(*ptr != iTAssignment) {
    throw std::runtime_error("decodeAssignment");
  }

  ptr++; // iTAssignment

  taco::Access lhs = decodeAccess(ptr);
  taco::IndexExpr rhs = decodeTExpr(ptr);
  return (lhs = rhs);
}

