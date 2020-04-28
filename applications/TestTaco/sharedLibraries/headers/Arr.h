#pragma once

#include <Object.h>
#include <Nothing.h>

struct Arr : public pdb::Object {
    void setUpAndCopyFrom(void* target, void* source) const {
        new (target) Arr();
        Arr& fromMe = *((Arr*)source);
        Arr& toMe   = *((Arr*)target);

        // copy the size
        toMe.size = fromMe.size;

        char* newLoc = (char*)toMe.data;
        char* data = (char*)fromMe.data;

        memmove(newLoc, data, fromMe.size);
    }

    void deleteObject(void* deleteMe) {
        deleter(deleteMe, this);
    }

    size_t getSize(void* forMe) {
        return sizeof(Arr) + ((Arr*)forMe)->size;
    }

    size_t size;
    pdb::Nothing data[0]; // TODO: nothing or char?
};

