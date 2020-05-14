# pragma once

#include "TUnOp.h"

struct TAbsOp: public TUnOp {
    ENABLE_DEEP_COPY

    TAbsOp() {}

    TAbsOp(Handle<TExpr> in)
        : TUnOp(in)
    {}

    taco::IndexExpr getAccess(
        std::vector<taco::TensorVar> const& tensorVars,
        std::vector<taco::IndexVar> const& indexVars) override
    {
        auto indexExprIn= expr->getAccess(tensorVars, indexVars);
        return taco::abs(indexExprIn);
    }
};
