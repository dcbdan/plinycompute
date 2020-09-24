#pragma once

#include "Exp.h"
#include "SetScanner.h"

class ExpScanner : public pdb::SetScanner<Exp> {
    using SetScanner = pdb::SetScanner<Exp>;
public:
    ENABLE_DEEP_COPY
    ExpScanner() = default;
    ExpScanner(const std::string &db, const std::string &set) : SetScanner(db, set) {}
};
