# pragma once

#include "TBinOp.h"

struct TMultOp: public TBinOp {
    ENABLE_DEEP_COPY

    TMultOp() {}

    TMultOp(Handle<TExpr> lIn, Handle<TExpr> rIn)
        : TBinOp(lIn, rIn)
    {}

    taco::IndexExpr getAccess(
        std::vector<taco::TensorVar> const& tensorVars,
        std::vector<taco::IndexVar> const& indexVars) override
    {
        auto left  = lhs->getAccess(tensorVars, indexVars);
        auto right = rhs->getAccess(tensorVars, indexVars);
        return left * right;
    }
};

