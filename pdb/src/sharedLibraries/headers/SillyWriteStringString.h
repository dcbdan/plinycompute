#pragma once

#include <SetWriter.h>
#include "StringStringPair.h"

namespace pdb {

class SillyWriteStringString : public SetWriter <StringStringPair> {

public:

  SillyWriteStringString() = default;

  SillyWriteStringString(const String &db_name, const String &set_name) : SetWriter(db_name, set_name) {}

  ENABLE_DEEP_COPY

};

}
