#pragma once

#include "Exp.h"
#include "SetWriter.h"

class ExpWriter : public pdb::SetWriter<Exp> {
    using SetWriter = pdb::SetWriter<Exp>;
public:
    ENABLE_DEEP_COPY
    ExpWriter() = default;
    ExpWriter(const std::string &db, const std::string &set) : SetWriter(db, set) {}
};
