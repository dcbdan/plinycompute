#include <PDBClient.h>
#include <ReadStringStringPair.h>
#include <SillyCartasianJoinStringString.h>
#include <SillyWriteStringString.h>
#include "StringStringPair.h"

using namespace pdb;

// some constants for the test
const size_t blockSize = 1;

size_t fillSetPageWithData(pdb::PDBClient &pdbClient, std::string const& setName) {
  size_t num = 0;

  // make the allocation block
  const pdb::UseTemporaryAllocationBlock tempBlock{blockSize * 1024 * 1024};

  // write a bunch of supervisors to it
  Handle <Vector <Handle <StringStringPair>>> data = pdb::makeObject<Vector <Handle <StringStringPair>>>();

  // fill the vector up
  size_t i = 0;
  for (; true; i++) {
    std::ostringstream ossV;
    ossV << "Value:" << i;
    std::ostringstream ossK;
    ossK << "Key:" << i;
    Handle <StringStringPair> myPair = makeObject<StringStringPair>(ossK.str(), ossV.str());
    data->push_back (myPair);

    if(i >= 100) {
      break;
    }
  }

  // how many did we have
  num = data->size();

  // send the data once
  pdbClient.sendData<StringStringPair>("myData", setName, data);

  return num;
}

int main(int argc, char* argv[]) {

  // make a client
  pdb::PDBClient pdbClient(8108, "localhost");

  /// 1. Register the classes

  // now, register a type for user data
  pdbClient.registerType("libraries/libStringStringPair.so");
  pdbClient.registerType("libraries/libReadStringStringPair.so");
  pdbClient.registerType("libraries/libSillyCartasianJoinStringString.so");
  pdbClient.registerType("libraries/libSillyWriteStringString.so");

  /// 2. Create the set

  // now, create a new database
  pdbClient.createDatabase("myData");

  // now, create the input and output sets
  pdbClient.createSet<StringStringPair>("myData", "mySetA");
  pdbClient.createSet<StringStringPair>("myData", "mySetB");
  pdbClient.createSet<StringStringPair>("myData", "outSet");

  /// 3. Fill in the data (single threaded)
  size_t numA = fillSetPageWithData(pdbClient, "mySetA");
  size_t numB = fillSetPageWithData(pdbClient, "mySetB");

  /// 4. Make query graph an run query

  // for allocations
  const UseTemporaryAllocationBlock tempBlock{1024 * 1024 * 128};

  // here is the list of computations
  Handle <Computation> readA = makeObject <ReadStringStringPair>("myData", "mySetA");
  Handle <Computation> readB = makeObject <ReadStringStringPair>("myData", "mySetB");
  Handle <Computation> join = makeObject <SillyCartasianJoinStringString>();
  join->setInput(0, readA);
  join->setInput(1, readB);
  Handle <Computation> write = makeObject <SillyWriteStringString>("myData", "outSet");
  write->setInput(0, join);

  //TODO this is just a preliminary version of the execute computation before we add back the TCAP generation
  pdbClient.executeComputations({ write });

  /// 5. Get the set from the
  size_t count = 0;
  auto it = pdbClient.getSetIterator<StringStringPair>("myData", "outSet");
  while(it->hasNextRecord()) {

    // grab the record
    Handle<StringStringPair> r = it->getNextRecord();

    // count the stuff
    count++;
  }

  // check the number returned
  if(count == numA * numB) {
    std::cout << "got correct number back " << count << " " << numA * numB;
  }
  else {
    std::cout << "got wrong number back " << count << " " << numA * numB;
  }
  std::cout << std::endl;

  // shutdown the server
  pdbClient.shutDownServer();

  return 0;
}
