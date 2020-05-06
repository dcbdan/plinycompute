# pragma once

#include "TBinOp.h"

struct TPlusOp: public TBinOp {
    ENABLE_DEEP_COPY

    TPlusOp() {}

    TPlusOp(Handle<TExpr> lIn, Handle<TExpr> rIn)
        : TBinOp(lIn, rIn)
    {}

    taco::IndexExpr getAccess(
        std::vector<taco::TensorVar> const& tensorVars,
        std::vector<taco::IndexVar> const& indexVars) override
    {
        auto left  = lhs->getAccess(tensorVars, indexVars);
        auto right = rhs->getAccess(tensorVars, indexVars);
        return left + right;
    }
};

