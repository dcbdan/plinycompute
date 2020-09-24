#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <map>

#include <PDBClient.h>
#include "sharedLibraries/headers/Tra.h"
#include "sharedLibraries/headers/DecodeTra.h"

#include <taco/format.h>
#include <taco/type.h>
#include <taco/tensor.h>

// This header is automatically generated from the
// toTra library. It offers the interfaces with haskell
// and includes HsFFI.h
#include "Foreign_stub.h"

using std::string;
using std::make_pair;
using std::pair;
using std::vector;

// This function marshals C++ inputs into Haskell via
// the hs_build_tra function. The hs_build_tra function returns a big sequence
// of integers that is parsed into a Tra computation, which is returned.
Handle<Computation> buildTra(
    std::string const& parseMe,
    std::map<std::string, int> const& modeToSize,
    std::map<std::string, int> const& modeToBlock,
    GetInput& getIn)
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
  Handle<Computation> out = nullptr;
  if(*decodeMe == -1) {
    std::cout << "Was not successful!" << std::endl;
  } else {
    // decodeTra will modify where decodeMe is pointed to
    out = decodeTra(decodeMe, getIn);
  }
  hs_free(freeMe);

  return out;
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

// For each of the meta values, call makeValue, and add it
// to the database
// (This function basically gets the index manipulations
//  out of the way)
void initTraTensorSet(
  pdb::PDBClient& pdbClient,
  std::string const& db,
  std::string const& set,
  pair<Type, Type> const& type, // Type = vector<pair<string, int>>
  std::function<Handle<TacoTensor>(Handle<TraMeta>&)> makeValue)
{
  std::vector<int> blockDim;
  for(int i = 0; i != type.first.size(); ++i) {
    blockDim.push_back(type.first[i].second);
  }

  // given the dimensions, create the Type object
  auto makeMeta = [&type](std::vector<int> const& block) {
    Type info;
    for(int i = 0; i != block.size(); ++i) {
      info.emplace_back(type.first[i].first, block[i]);
    }

    Handle<TraMeta> out = makeObject<TraMeta>(info);
    return out;
  };

  // count the number of blocks
  int numBlocks = 1;
  for(int i = 0; i != blockDim.size(); ++i) {
      numBlocks *= blockDim[i];
  }

  int blockId = 0;
  std::vector<int> block;
  while(blockId != numBlocks) {
    const pdb::UseTemporaryAllocationBlock tempBlock{256 * 1024 * 1024};

    using VecBlock = Vector<Handle<TraTensor>>;

    // put each block here
    Handle<VecBlock> data = pdb::makeObject<VecBlock>();

    try {
        for(; blockId != numBlocks; ++blockId) {
            block.clear();
            auto index=blockId;
            for(size_t mode = 0; mode < blockDim.size()-1; mode++) {
                block.push_back(index%blockDim[mode]);
                index=index/blockDim[mode];
            }
            block.push_back(index);

            Handle<TraMeta> meta = makeMeta(block);
            Handle<TacoTensor> value = makeValue(meta);

            data->push_back(makeObject<TraTensor>(value, meta));
        }
    } catch(pdb::NotEnoughSpace &n) {
    }

    // init the records
    getRecord(data);

    // send the data
    pdbClient.sendData<TraTensor>(db, set, data);
  }
}

void initEmpty(
  pdb::PDBClient& pdbClient,
  std::string const& db,
  std::string const& set,
  pair<Type, Type> const& type) // Type = vector<pair<string, int>>
{
  std::function<Handle<TacoTensor>(Handle<TraMeta>&)> makeValue =
    [](Handle<TraMeta>&) {
      return nullptr;
    };

  initTraTensorSet(pdbClient, db, set, type, makeValue);
}

template <typename Int>
struct IterateCoord {
    IterateCoord(std::vector<Int> begIn, std::vector<Int> endIn)
        : beg(begIn), end(endIn)
    {}

    bool getNext(std::vector<Int>& coord) const {
        while(true) {
            coord.back() += 1;
            if(coord.back() == end[coord.size()-1]) {
                coord.pop_back();
            } else {
                break;
            }

            if(coord.size() == 0) {
                return false;
            }
        }

        auto order = beg.size();
        while(coord.size() != order) {
            coord.push_back(beg[coord.size()]);
        }

        return true;
    }

private:
    std::vector<Int> beg;
    std::vector<Int> end;
};

void initRandom(
  pdb::PDBClient& pdbClient,
  std::string const& db,
  std::string const& set,
  pair<Type, Type> const& type) // Type = vector<pair<string, int>>
{
  Type const& tensorType = type.second;
  size_t order = tensorType.size();

  // random generator stuff
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<double> rnorm;
  auto genVal = [&]() {
      return rnorm(gen);
  };
  std::vector<int> beg(order, 0);
  std::vector<int> end;
  for(int i = 0; i != order; ++i) {
    end.push_back(tensorType[i].second);
  }

  taco::Format format(std::vector<taco::ModeFormatPack>(order, taco::dense));

  // Add a random value to each element in a taco::TensorBase
  // and then copy it to pdb and return
  std::function<Handle<TacoTensor>(Handle<TraMeta>&)> makeValue =
    [&](Handle<TraMeta>&) {
      taco::TensorBase out(taco::type<double>(), end, format);
      IterateCoord<int> iter(beg, end);
      std::vector<int> coord = beg;
      do {
        out.insert(coord, genVal());
      } while(iter.getNext(coord));

      out.pack();

      // now copy it into pdb
      Handle<TacoTensor> outTacoTensor = makeObject<TacoTensor>(out);
      return outTacoTensor;
    };

  initTraTensorSet(pdbClient, db, set, type, makeValue);
}

void registerTra(pdb::PDBClient & pdbClient) {
  // TacoTensor is used as the value type
  // and it requires Arr
  pdbClient.registerType("libraries/libArr.so");
  pdbClient.registerType("libraries/libTacoTensor.so");

  // And the Tra types
  pdbClient.registerType("libraries/libTraAgg.so");
  pdbClient.registerType("libraries/libTraFilter.so");
  pdbClient.registerType("libraries/libTraJoin.so");
  pdbClient.registerType("libraries/libTraMeta.so");
  pdbClient.registerType("libraries/libTraRekey.so");
  pdbClient.registerType("libraries/libTraScanner.so");
  pdbClient.registerType("libraries/libTraTensor.so");
  pdbClient.registerType("libraries/libTraTransform.so");
  pdbClient.registerType("libraries/libTraWriter.so");
}

int main() {
  // launch the Haskell runtime
  hs_init(0, nullptr);

  // make a client
  pdb::PDBClient pdbClient(8108, "localhost");

  // register the types
  registerTra(pdbClient);

  const pdb::UseTemporaryAllocationBlock tempBlock{512 * 1024 * 1024};

  // now, create a new database
  std::string const db = "TestHs";
  pdbClient.createDatabase(db);

  pdbClient.createSet<TraTensor>(db, "A");
  pdbClient.createSet<TraTensor>(db, "B");
  pdbClient.createSet<TraTensor>(db, "C");

  GetInput getIn = [&](
      std::string const& name,
      pair<Type, Type> const& type)
  {
    initRandom(pdbClient, db, name, type);
    Handle<Computation> out = makeObject<TraScanner>(db, name);
    return out;
  };

  Handle<Computation> out = makeObject<TraWriter>(db, "C");
  Handle<Computation> matMul = buildTra(
    "(i,k){A(i,k) * B(j,k)}",
    constructMap({"i", "j", "k"}, {1000, 1100, 1200}),
    constructMap({"i", "j", "k"}, {20, 20, 20}),
    getIn);

  std::cout << "-----------------------------------------" << std::endl;
  if(!matMul.isNullPtr()) {
    out->setInput(matMul);
    pdbClient.executeComputations({ out });
  } else {
    std::cout << "Error: did not construct matMul from buildTra!" << std::endl;
  }

  // shutdown haskell
  hs_exit();
  // shutdown the server
  pdbClient.shutDownServer();
}

