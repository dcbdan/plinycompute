#include <iostream>
#include <vector>
#include <string>
#include <map>

#include "Foreign_stub.h"

// TODO: make this an enum or in a namespace
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

using std::string;
using std::make_pair;
using std::pair;
using std::vector;

using Type = vector<pair<string, int>>;

// This is tedious... The decode is how I take algebraic data structures
// from haskell and put them into C++ land.
// On the haskell side, there is an encode function that takes everything
// and turns it into a big sequence of integers.
// The decode takes a big sequence of integers and reads it into
// whatever it is meant to represent.
void decodeTra(int*& ptr);
// All of these decode functions take as input a reference to a pointer--
// so that each decode intentionally consumes (increments) ptr. When it
// completes, it leaves the pointer ready to parse the next thing.

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

vector<pair<string, string> > decodeKeyPairs(int*& ptr) {
  int numToGet = *ptr++;
  vector<pair<string, string>> ret;
  for(int cnt = 0; cnt != numToGet; ++cnt) {
    ret.push_back(make_pair(decodeStr(ptr), decodeStr(ptr)));
  }
  return ret;
}

void decodeTraTensor(int*& ptr) {
  if(*ptr != iTensor) {
    throw std::runtime_error("TraTensor");
  }
  std::cout << "TraTensor  ";
  ptr++; // remove iTensor
  decodeRel(ptr);
  std::cout << decodeStr(ptr) << std::endl;
}

void decodeTraAgg(int*& ptr) {
  if(*ptr != iAgg) {
    throw std::runtime_error("TraAgg");
  }
  std::cout << "TraAgg   ";
  ptr++;
  decodeRel(ptr);
  while(*ptr == iStr) {
    std::cout << decodeStr(ptr) << "  ";
  }
  std::cout << std::endl;

  // get the expr
  ptr++;

  decodeTra(ptr);
}

void decodeTraJoin(int*& ptr) {
  if(*ptr != iJoin) {
    throw std::runtime_error("TraJoin");
  }
  std::cout << "TraJoin" << std::endl;
  ptr++;
  decodeRel(ptr);
  decodeKeyPairs(ptr);

  // get the expr
  ptr++;

  decodeTra(ptr);
  decodeTra(ptr);
}

void decodeTraTransform(int*& ptr) {
  if(*ptr != iTransform) {
    throw std::runtime_error("TraTransform");
  }
  std::cout << "TraTransform" << std::endl;
  ptr++;
  decodeRel(ptr);

  // get the expr
  ptr++;

  decodeTra(ptr);
}

void decodeTraRekey(int*& ptr) {
  if(*ptr != iRekey) {
    throw std::runtime_error("TraRekey");
  }
  std::cout << "TraRekey" << std::endl;
  ptr++;
  decodeRel(ptr);

  // get the keyop
  ptr++;

  decodeTra(ptr);
}

void decodeTraFilterEqKey(int*& ptr) {
  if(*ptr != iFilterEqKey) {
    throw std::runtime_error("TraFilterEqKey");
  }
  std::cout << "TraFilterEqKey" << std::endl;
  ptr++;
  decodeRel(ptr);

  decodeKeyPairs(ptr);

  decodeTra(ptr);
}

void decodeTra(int*& ptr) {
  if(*ptr == iTensor) {
    return decodeTraTensor(ptr);
  }
  if(*ptr == iAgg) {
    return decodeTraAgg(ptr);
  }
  if(*ptr == iJoin) {
    return decodeTraJoin(ptr);
  }
  if(*ptr == iTransform) {
    return decodeTraTransform(ptr);
  }
  if(*ptr == iRekey) {
    return decodeTraRekey(ptr);
  }
  if(*ptr == iFilterEqKey) {
    return decodeTraFilterEqKey(ptr);
  }

  throw std::runtime_error("decodeTra");
}

// This function marshals C++ inputs into Haskell via
// the hs_build_tra function. The hs_build_tra function returns a big sequence
// of integers that is parsed into a Tra computation, which is returned.
std::vector<int> buildTra(
    std::string const& parseMe,
    std::map<std::string, int> const& modeToSize,
    std::map<std::string, int> const& modeToBlock)
{
  // copy over all the strings here
  std::vector<std::string> copyHere;
  copyHere.reserve(modeToSize.size() + modeToBlock.size());

  std::vector<const char*> modes;
  std::vector<int> sizes;
  for(auto modeSizePair: modeToSize) {
    copyHere.push_back(modeSizePair.first);
    modes.push_back(copyHere.back().c_str());
    sizes.push_back(modeSizePair.second);
  }

  std::vector<const char*> blockModes;
  std::vector<int> blockSizes;
  for(auto modeSizePair: modeToBlock) {
    copyHere.push_back(modeSizePair.first);
    blockModes.push_back(copyHere.back().c_str());
    blockSizes.push_back(modeSizePair.second);
  }
  // At this point, modes and blockModes just contains memory pointed
  // to in copyHere (it should be a tiny amount of memory, so whatever)
  // .. But modes.data() and blockModes.data() is of the Ptr CString
  //    type needed

  // CString -> CInt -> Ptr CString -> Ptr CInt
  //         -> CInt -> Ptr CString -> Ptr CInt -> IO (Ptr CInt)
  int* decodeMe = (int*)hs_build_tra(
      (void*)parseMe.c_str(),
      int(modes.size()), (void*)modes.data(), (void*)sizes.data(),
      int(blockModes.size()), (void*)blockModes.data(), (void*)blockSizes.data());
  // copy decodeMe into freeMe so that freeMe holds the initial value for
  // when the data needs to be freed
  int* freeMe = decodeMe;

  // copy over everything we need to
  // and then free decodeMe
  if(*decodeMe == -1) {
    std::cout << "Was not successful!" << std::endl;
  } else {
    // decodeTra will modify where decodeMe is pointed to
    decodeTra(decodeMe);
  }
  hs_free(freeMe);

  return {};
}

// no templates to use brace-initalizer
std::map<std::string, int> constructMap(
  std::vector<std::string> const& keys,
  std::vector<int> const& values)
{
  if(keys.size() != values.size())
    throw std::runtime_error("constructMap requires same length inputs");
  std::map<std::string, int> ret;
  for(int i = 0; i != keys.size(); ++i) {
      ret[keys[i]] = values[i];
  }
  return ret;
}

int main() {
  hs_init(0, nullptr);
  //std::cout << "Hello from C++" << std::endl;
  //helloFromHaskell();

  //int* v = (int*)encode_tra();
  //std::cout << v[0] << ", " << v[1] << std::endl;
  //hs_free(v);

  buildTra(
      "(i,k){A(i,k) * B(j,k)}",
      constructMap({"i", "j", "k"}, {10, 11, 12}),
      constructMap({"i", "j", "k"}, {2, 2, 2}));

  hs_exit();
}
