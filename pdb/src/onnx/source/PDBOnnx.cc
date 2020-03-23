#ifndef PDBONNX_CC
#define PDBONNX_CC

#include "PDBOnnx.h"
#include <iostream>

namespace pdb {

  void PDBOnnx::helloWorld() {
    std::cout << "Hello, World!" << std::endl;
  }

  void PDBOnnx::doSomething() {
//    std::cout << "Register type..." << std::endl;
//    pdbClient.registerType("../libraries/libMatrixBlock.so");

    std::cout << "Create database..." << std::endl;
    pdbClient.createDatabase("myData");

    std::cout << "Create set..." << std::endl;
    pdbClient.createSet<int>("myData", "A");
  }

  void PDBOnnx::shutDown() {
    pdbClient.shutDownServer();
  }
}

#endif
