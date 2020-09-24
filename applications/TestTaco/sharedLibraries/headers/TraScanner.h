#pragma once

#include "TraTensor.h"
#include "SetScanner.h"

class TraScanner : public pdb::SetScanner<TraTensor> {
    using SetScanner = pdb::SetScanner<TraTensor>;
public:
    ENABLE_DEEP_COPY
    TraScanner() = default;
    TraScanner(const std::string &db, const std::string &set) : SetScanner(db, set) {}
};
