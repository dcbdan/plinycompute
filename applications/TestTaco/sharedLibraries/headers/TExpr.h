#pragma once

#include <vector>

#include <Object.h>
#include <PDBVector.h>

#include <taco/tensor.h>

using namespace pdb;

struct TTensor;
struct TPlusOp;
struct TMultOp;
struct TBinOp;

struct TExpr : public Object {
    ENABLE_DEEP_COPY

    TExpr() {}

    virtual taco::IndexExpr getAccess(
        std::vector<taco::TensorVar> const& tensorVars,
        std::vector<taco::IndexVar> const& indexVars)
    {
        std::cout << "This function should be overriden!" << std::endl;
        exit(1);
    }

    virtual void getTensors(std::vector<TTensor*>& ttensors)
    {
        std::cout << "This function should be overriden!" << std::endl;
        exit(1);
    }
};

