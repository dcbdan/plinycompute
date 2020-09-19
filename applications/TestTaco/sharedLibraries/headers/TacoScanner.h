#pragma once

#include "TacoTensorBlock.h"
#include "SetScanner.h"

class TacoScanner : public pdb::SetScanner<TacoTensorBlock> {
    using SetScanner = pdb::SetScanner<TacoTensorBlock>;
public:
    ENABLE_DEEP_COPY
    TacoScanner() = default;
    TacoScanner(const std::string &db, const std::string &set) : SetScanner(db, set) {}
};
