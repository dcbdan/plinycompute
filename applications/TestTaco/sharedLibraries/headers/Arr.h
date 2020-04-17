#pragma once

#include <Object.h>
#include <Nothing.h>

struct Arr : public pdb::Object {

    ENABLE_DEEP_COPY

    size_t size;
    pdb::Nothing data[0]; // TODO: nothing or char?
};

