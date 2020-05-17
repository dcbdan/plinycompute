# pragma once

#include "TUnOp.h"

struct TSigmoidOp: public TUnOp {
    ENABLE_DEEP_COPY

    TSigmoidOp() {}

    TSigmoidOp(Handle<TExpr> in)
        : TUnOp(in)
    {}

    taco::IndexExpr getAccess(
        std::vector<taco::TensorVar> const& tensorVars,
        std::vector<taco::IndexVar> const& indexVars) override
    {
        auto indexExprIn= expr->getAccess(tensorVars, indexVars);
        return 1.0 / (1.0 + taco::exp(-1.0*indexExprIn));
    }
};
