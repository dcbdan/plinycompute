#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <map>

#include <PDBClient.h>

#include "sharedLibraries/headers/ExpJoin.h"
#include "sharedLibraries/headers/Exp.h"
#include "sharedLibraries/headers/ExpScanner.h"
#include "sharedLibraries/headers/ExpWriter.h"

void makeSet(PDBClient& pdbClient, std::string db, std::string set) {
  const pdb::UseTemporaryAllocationBlock tempBlock{512 * 1024 * 1024};

  int n = 100;
  Handle<Vector<Handle<Exp>>> data = makeObject<Vector<Handle<Exp>>>(n, 0);

  std::string key = "ads";
  std::string value = "dsa";

  for(int i = 0; i != n; ++i) {
    data->push_back(makeObject<Exp>(key, value));
  }

  getRecord(data);

  pdbClient.sendData<Exp>(db, set, data);
}

int main() {
  // make a client
  pdb::PDBClient pdbClient(8108, "localhost");

  pdbClient.registerType("libraries/libExp.so");
  pdbClient.registerType("libraries/libExpJoin.so");
  pdbClient.registerType("libraries/libExpScanner.so");
  pdbClient.registerType("libraries/libExpWriter.so");

  const pdb::UseTemporaryAllocationBlock tempBlock{512 * 1024 * 1024};

  std::string const db = "dbExp";

  pdbClient.createSet<Exp>(db, "A");
  pdbClient.createSet<Exp>(db, "B");
  pdbClient.createSet<Exp>(db, "C");

  makeSet(pdbClient, db, "A");
  makeSet(pdbClient, db, "B");

  Handle<Computation> A = makeObject<ExpScanner>(db, "A");
  Handle<Computation> B = makeObject<ExpScanner>(db, "B");

  Handle<Computation> J = makeObject<ExpJoin>();
  J->setInput(0, A);
  J->setInput(1, B);

  Handle<Computation> out = makeObject<ExpWriter>(db, "C");
  out->setInput(J);

  pdbClient.executeComputations({ out });

  auto iter = pdbClient.getSetIterator<Exp>(db, "A");

  if(iter == nullptr) {
    std::cout << "Error: nullptr iterator" << std::endl;
  } else {
    int count = 0;
    while(iter->hasNextRecord()) {
      count++;
      iter->getNextRecord();
    }
    std:cout << "The count: " << count << std::endl;
  }

  // shutdown the server
  pdbClient.shutDownServer();
}
