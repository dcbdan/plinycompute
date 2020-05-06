#pragma once

#include "TExpr.h"

struct TBinOp: public TExpr {
    ENABLE_DEEP_COPY

    TBinOp() {}

    TBinOp(Handle<TExpr> lIn, Handle<TExpr> rIn)
        : lhs(lIn), rhs(rIn)
    {}

    void getTensors(std::vector<TTensor*>& ttensors) override {
        lhs->getTensors(ttensors);
        rhs->getTensors(ttensors);
    }

    Handle<TExpr> lhs;
    Handle<TExpr> rhs;
};

