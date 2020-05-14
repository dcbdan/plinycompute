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

    // A "free" index is an index that is on the lhs. so A(i,j) = B(i,k)*C(k,j) has
    // i,j as free.
    // numIndex is the total number of indices, so i,j,k are the 3 indices from above
    TAssignment(int numFreeIn, int numIndexIn, Handle<TTensor> tIn, Handle<TExpr> eIn)
        : lhs(tIn), rhs(eIn), numFree(numFreeIn), numIndex(numIndexIn)
    {
        // get all the leaf nodes on the right side
        std::vector<TTensor*> ttensors;
        rhs->getTensors(ttensors);

        // this is the number of join arguments to use
        numExprLeafNodes = ttensors.size();

        setEquals(ttensors);
        setOut(ttensors);
    }

    taco::Assignment getAssignment(
        std::vector<taco::TensorVar> const& tensorVars)
    {
        // TODO: if tensorVars is too small, throw error
        std::vector<taco::IndexVar> indexVars(numIndex);
        auto left = lhs->operator()(tensorVars[0], indexVars);
        auto right = rhs->getAccess(tensorVars, indexVars);
        return (left = right);
    }

    void diagnostic() {
        std::cout << "numFree " << numFree << std::endl;
        std::cout << "numIndex " << numIndex << std::endl;
        std::cout << "numExprLeafNodes " << numExprLeafNodes << std::endl;

        for(int i = 0; i != whichInputL.size(); ++i) {
            std::cout << "(" << whichInputL[i] << ", " <<
                                whichIndexL[i] << ", " <<
                                whichInputR[i] << ", " <<
                                whichIndexR[i] << ") " << std::endl;
        }

        for(int i = 0; i != whichOut.size(); ++i) {
            std::cout << "(" << whichOut[i]      << ", " <<
                                whichOutIndex[i] << ") " << std::endl;
        }
    }

private:
    void setOut(std::vector<TTensor*> const& ttensors) {
        // Here is the idea. suppose that Out(i,j) = A(i,k)*B(k,l)*C(l,j).
        // Then (whichOut, whichOutIndex) should be
        //    (1,0) and (3,1).
        // So that thejoin can find the output block size
        whichOut = Vector<int>(numFree);
        whichOutIndex = Vector<int>(numFree);
        for(int i = 0; i != numFree; ++i) {
            whichOut.push_back(-1);
            whichOutIndex.push_back(-1);
        }

        for(int i = 0; i != ttensors.size(); ++i) {
            TTensor* t = ttensors[i];

            for(int idx = 0; idx != t->whichIdxs.size(); ++idx) {
                int ti = t->whichIdxs[idx];
                // we could stop once we found them all, but the number
                // of tensors is so small it shouldn't matter
                if(ti < numFree) {
                    whichOut[ti] = t->which;
                    whichOutIndex[ti] = idx;
                }
            }
        }

        // make sure that all the index locations have been set
        for(int i = 0; i != numFree; ++i) {
            if(whichOut[i] == -1) {
                std::cout << "whichOut[" << i << "] was not set!" << std::endl;
            }
        }
    }

    void setEquals(std::vector<TTensor*> const& ttensors) {
        // Here is the idea. Suppose we have A(i,j) = B(i,k)*C(k,j) + D(i,j)
        // Then this sets whichInputX and whichIndexX to contain
        // how the join should occur across B and C. so
        // if Ts = {A,B,C,D}, then
        //   Ts[whichInputL[i]].index[whichIndexL[i]] ==
        //   Ts[whichInputR[i]].index[whichIndexR[i]]
        // for i = 1, ..., whichInputL.size()... In this case,
        // whichInputL.size() would be 3 for
        //   Ts[1].index[1] = Ts[2].index[0], (k)
        //   Ts[1].index[0] = Ts[3].index[0], (i)
        //   Ts[2].index[1] = Ts[3].index[1], (j)

        // assert that lhs->whichIdxs are all less than numFree
        for(int i = 0; i != lhs->whichIdxs.size(); ++i) {
            if(lhs->whichIdxs[i] >= numFree) {
                std::cout << "Oh no!" << std::endl;
                exit(1); // TODO
            }
        }

        // for each pair of leaf nodes, see if they have any matching index variables
        // TODO: the nested for loops are ugly and confusing
        for(int i = 0; i != ttensors.size()-1; ++i) {
            TTensor* t1 = ttensors[i];

            // get all of the indices in the first
            std::map<int, int> t1m;
            for(int idx = 0; idx != t1->whichIdxs.size(); ++idx) {
                int t1i = t1->whichIdxs[idx];
                t1m[t1i] = idx;
            }
            for(int j = i+1; j != ttensors.size(); ++j) {
                TTensor* t2 = ttensors[j];
                for(int idx = 0; idx != t2->whichIdxs.size(); ++idx) {
                    int t2i = t2->whichIdxs[idx];
                    if(t1m.count(t2i) > 0) {
                        // a summed index is in both t1 and t2, therefore
                        // the equality check must be added

                        whichInputL.push_back(t1->which);
                        whichInputR.push_back(t2->which);
                        whichIndexL.push_back(t1m[t2i]);
                        whichIndexR.push_back(idx);
                    }
                }
            }
        }
    }

public:
    Handle<TTensor> lhs;
    Handle<TExpr> rhs;
    int numFree;
    int numIndex;
    int numExprLeafNodes;

    // These are used by a join to make sure all summed indices are equal
    // tuple here? or pair of pairs? TODO
    Vector<int> whichInputL;
    Vector<int> whichInputR;
    Vector<int> whichIndexL;
    Vector<int> whichIndexR;

    // These are used by a join to determine where to find the free indices
    // block size
    // also, use pair<int, int>? TODO
    Vector<int> whichOut;
    Vector<int> whichOutIndex;
};


