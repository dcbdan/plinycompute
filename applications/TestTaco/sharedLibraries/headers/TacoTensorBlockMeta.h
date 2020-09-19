#pragma once

#include <Object.h>
#include <PDBVector.h>

class TacoTensorBlockMeta : public pdb::Object {
public:
    // Default to a block of order 0
    TacoTensorBlockMeta(): idxs() {
    }

    ENABLE_DEEP_COPY

    // Just set all the values of idxs to idxsIn
    TacoTensorBlockMeta(std::vector<uint32_t> const& idxsIn)
      : idxs(idxsIn.size(), idxsIn.size()) {
        for(int i = 0; i != idxsIn.size(); ++i) {
            idxs[i] = idxsIn[i];
        }
    }

    // To access modes counting from the back, use negative numebrs.
    // For example, to access the last mode, use -1
    uint32_t access(int which) {
        if(which < 0) {
            return idxs[idxs.size() - which];
        } else {
            return idxs[which];
        }
    }

    template <int which>
    uint32_t access() {
        return access(which);
    }

    bool operator==(const TacoTensorBlockMeta &other) const {
        if(idxs.size() != other.idxs.size()) {
            return false;
        }
        for(size_t i = 0; i != idxs.size(); ++i) {
            if(idxs[i] != other.idxs[i]) {
                return false;
            }
        }
        return true;
    };

    size_t hash() const {
        size_t mult = 10000;
        size_t ret = 0;
        for(size_t i = 0; i != idxs.size(); ++i) {
            ret += mult*idxs[i];
            mult *= 10;
            mult += 7;
        }
        return ret;
    }

    pdb::Vector<uint32_t> const& getBlock() const {
        return idxs;
    }
public:
    pdb::Vector<uint32_t> idxs;
};
