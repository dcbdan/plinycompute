# pragma once

#include "TExpr.h"

using std::vector;

struct TTensor: public TExpr {
    ENABLE_DEEP_COPY

    TTensor() {}

    TTensor(int whichIn, vector<int> whichIdxsIn)
        : which(whichIn), whichIdxs(whichIdxsIn.size())
    {
        for(int const& i: whichIdxsIn) {
            whichIdxs.push_back(i);
        }
    }

    void getTensors(std::vector<TTensor*>& ttensors) override {
        // keep ttensors full of unique values
        for(TTensor* t: ttensors) {
        // this does not scale but since the final size of ttensors
        // will be small, it shouldn't matter
            if(*t == *this) {
                return;
            }
        }

        ttensors.push_back(this);
    }

    taco::IndexExpr getAccess(
        std::vector<taco::TensorVar> const& tensorVars,
        std::vector<taco::IndexVar> const& indexVars) override
    {
        taco::TensorVar var = tensorVars[which];
        return this->operator()(var, indexVars);
    }

    taco::Access operator()(
        taco::TensorVar const& tensorVar,
        std::vector<taco::IndexVar> const& indexVars)
    {
        std::vector<taco::IndexVar> myVar;

        for(int i = 0; i != whichIdxs.size(); ++i) {
            auto whichIdx = whichIdxs[i];
            myVar.push_back(indexVars[whichIdx]);
        }

        return tensorVar(myVar);
    }

    bool operator==(TTensor const& other) const {
        if(which != other.which) {
            return false;
        }
        if(whichIdxs.size() != other.whichIdxs.size()) {
            return false;
        }
        for(int i = 0; i != whichIdxs.size(); ++i) {
            if(whichIdxs[i] != other.whichIdxs[i]) {
                return false;
            }
        }
        return true;
    }

    bool requiresDenseOutput() override {
        return false;
    }

    int which;              // this is which tensor to use
    Vector<int> whichIdxs;  // this is which IndexVar to use
};
