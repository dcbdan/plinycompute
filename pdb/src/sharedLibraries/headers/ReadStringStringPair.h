#ifndef PDB_ReadStringStringPair_H
#define PDB_ReadStringStringPair_H

#include <SetScanner.h>
#include <StringStringPair.h>

namespace pdb {

class ReadStringStringPair : public SetScanner <StringStringPair> {
public:

  ReadStringStringPair() = default;

  ReadStringStringPair(const std::string &db, const std::string &set) : SetScanner(db, set) {}

  ENABLE_DEEP_COPY
};



}

#endif //PDB_ReadStringStringPair_H
