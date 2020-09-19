#pragma once

#include "TExpr.h"

struct TUnOp: public TExpr {
    ENABLE_DEEP_COPY

    TUnOp() {}

    TUnOp(Handle<TExpr> in)
        : expr(in)
    {}

    void getTensors(std::vector<TTensor*>& ttensors) override {
        expr->getTensors(ttensors);
    }

    bool requiresDenseOutput() override {
        return expr->requiresDenseOutput();
    }

    Handle<TExpr> expr;
};
