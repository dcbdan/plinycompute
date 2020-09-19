#pragma once

#include "TacoTensorBlock.h"
#include "SetWriter.h"

class TacoWriter : public pdb::SetWriter<TacoTensorBlock> {
    using SetWriter = pdb::SetWriter<TacoTensorBlock>;
public:
    ENABLE_DEEP_COPY
    TacoWriter() = default;
    TacoWriter(const std::string &db, const std::string &set) : SetWriter(db, set) {}
};
