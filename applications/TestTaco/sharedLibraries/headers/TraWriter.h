#pragma once

#include "TraTensor.h"
#include "SetWriter.h"

class TraWriter : public pdb::SetWriter<TraTensor> {
    using SetWriter = pdb::SetWriter<TraTensor>;
public:
    ENABLE_DEEP_COPY
    TraWriter() = default;
    TraWriter(const std::string &db, const std::string &set) : SetWriter(db, set) {}
};
