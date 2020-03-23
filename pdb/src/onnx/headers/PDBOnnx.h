#ifndef PDBONNX_H
#define PDBONNX_H

#include "PDBClient.h"
#include <string>

#include "PDBVector.h"

namespace pdb {

class PDBOnnx {
 public:

  /**
   * Creates PDBOnnx
   * @param portIn - the port number of the PDB manager server
   * @param addressIn - the IP address of the PDB manager server
   */
  PDBOnnx(int portIn, std::string addressIn)
    : pdbClient(portIn, addressIn)
  {}

  void doSomething();

  void shutDown();

  void helloWorld();

 private:
  PDBClient pdbClient;
};

}

#endif
