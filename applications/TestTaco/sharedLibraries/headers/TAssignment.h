#pragma once

#include <vector>

#include <Object.h>
#include <PDBVector.h>

#include "TTensor.h"
#include "TExpr.h"

#include <taco/tensor.h>

using namespace pdb;

struct TAssignment : public Object {
    ENABLE_DEEP_COPY

    TAssignment() {}

    TAssignment(int numFreeIn, int numIndexIn, Handle<TTensor> tIn, Handle<TExpr> eIn)
        : lhs(tIn), rhs(eIn), numFree(numFreeIn), numIndex(numIndexIn)
    {
        setEquals();
    }

    taco::Assignment getAssignment(
        std::vector<taco::TensorVar> const& tensorVars)
    {
        std::vector<taco::IndexVar> indexVars(numIndex);
        auto left = lhs->operator()(tensorVars[0], indexVars);
        auto right = rhs->getAccess(tensorVars, indexVars);
        return (left = right);
    }

    void setEquals() {
        // Here is the idea. suppose that Output = AST(T1, ..., T5).
        // I want to guarantee that all free variables in Output
        // are equal across T1, ..., T5.
        // suppose TS = {T1, ..., T5}. Then
        //   Ts[whichInputL[i]].index[whichIndexL[i]] ==
        //   Ts[whichInputR[i]].index[whichIndexR[i]]
        // for i = 1, ..., Ts.size().

        // assert that lhs->whichIdxs are all less than numFree
        for(int i = 0; i != lhs->whichIdxs.size(); ++i) {
            if(lhs->whichIdxs[i] >= numFree) {
                std::cout << "Oh no!" << std::endl;
                exit(1); // TODO
            }
        }

        // get all the leaf nodes
        std::vector<TTensor*> ttensors;
        rhs->getTensors(ttensors);

        // for each pair of leaf nodes, see if they have any matching index variables
        // TODO: the nested for loops are really ugly and confusing
        for(int i = 0; i != ttensors.size()-1; ++i) {
            TTensor* t1 = ttensors[i];
            // get all of the indices in the first
            std::map<int, int> t1m;
            for(int idx = 0; idx != t1->whichIdxs.size(); ++idx) {
                int t1i = t1->whichIdxs[idx];
                if(t1i >= numFree) {
                    t1m[t1i] = idx;
                }
            }
            for(int j = i+1; j != ttensors.size(); ++j) {
                TTensor* t2 = ttensors[j];
                for(int idx = 0; idx != t2->whichIdxs.size(); ++idx) {
                    int t2i = t2->whichIdxs[idx];
                    if(t1m.count(t2i) > 0) {
                        // a summed index is in both t1 and t2, therefore
                        // the equality check must be added
                        whichInputL.push_back(i);
                        whichInputR.push_back(j);
                        whichIndexL.push_back(t1m[t2i]);
                        whichIndexR.push_back(idx);
                    }
                }
            }
        }
    }

    Handle<TTensor> lhs;
    Handle<TExpr> rhs;
    int numFree;
    int numIndex;

    // These are used by a join to make sure all summed indices are equal
    // tuple here? or pair of pairs? TODO
    Vector<int> whichInputL;
    Vector<int> whichInputR;
    Vector<int> whichIndexL;
    Vector<int> whichIndexR;
};


