# pragma once

#include "TUnOp.h"

struct TReluOp: public TUnOp {
    ENABLE_DEEP_COPY

    TReluOp() {}

    TReluOp(Handle<TExpr> in)
        : TUnOp(in)
    {}

    taco::IndexExpr getAccess(
        std::vector<taco::TensorVar> const& tensorVars,
        std::vector<taco::IndexVar> const& indexVars) override
    {
        auto indexExprIn= expr->getAccess(tensorVars, indexVars);
        // TODO: incorporate type of tensorVars... either 0 or 0.0
        return taco::max(0.0, indexExprIn);
    }

    bool requiresDenseOutput() override {
        return true; // TODO: this should be false! but another
                     //       relu implementation would be needed
    }
};
