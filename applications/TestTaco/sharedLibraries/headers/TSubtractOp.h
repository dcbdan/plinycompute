# pragma once

#include "TBinOp.h"

struct TSubtractOp: public TBinOp {
    ENABLE_DEEP_COPY

    TSubtractOp() {}

    TSubtractOp(Handle<TExpr> lIn, Handle<TExpr> rIn)
        : TBinOp(lIn, rIn)
    {}

    taco::IndexExpr getAccess(
        std::vector<taco::TensorVar> const& tensorVars,
        std::vector<taco::IndexVar> const& indexVars) override
    {
        auto left  = lhs->getAccess(tensorVars, indexVars);
        auto right = rhs->getAccess(tensorVars, indexVars);
        return left - right;
    }
};
