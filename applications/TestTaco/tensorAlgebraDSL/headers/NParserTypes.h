#ifndef PARSER_TYPES_H_ASD
#define PARSER_TYPES_H_ASD

// Here we define all of the structs that are
// passed around by the parser

#include <memory>
#include <string>
#include <vector>
#include <iostream>

// This feels sort of dumb. The AST defined
// in the sharedLibriraies is almost the
// same as that for the parser below.
// The difference is that this AST is all
// pdb objects and it's purpose is to be used
// to construct a taco::Assignment statement
// given some tensorVars.
#include <../../sharedLibraries/headers/TParserTypes.h>

// Include join, aggregation, writer, scanner
#include <../../sharedLibraries/headers/TacoCompute.h>

#include "Computation.h"

using namespace pdb;

using std::string;
using std::shared_ptr;
using std::make_shared;
using std::vector;

struct NNode;
struct NExpr;
struct NTensor;
struct NConstant;
struct NBinOp;
struct NMultOp;
struct NPlusOp;
struct NInput;
struct NOutput;
struct NAssignment;
struct NProgram;

using NNodePtr        = shared_ptr<NNode>;
using NExprPtr        = shared_ptr<NExpr>;
using NTensorPtr      = shared_ptr<NTensor>;
using NConstantPtr    = shared_ptr<NConstant>;
using NBinOpPtr       = shared_ptr<NBinOp>;
using NMultOpPtr      = shared_ptr<NMultOp>;
using NPlusOpPtr      = shared_ptr<NPlusOp>;
using NInputPtr       = shared_ptr<NInput>;
using NOutputPtr      = shared_ptr<NOutput>;
using NAssignmentPtr  = shared_ptr<NAssignment>;
using NProgramPtr     = shared_ptr<NProgram>;

// Anywhere a pointer is passed into a constructor,
// the ownership of the pointer is conceeded

struct NNode {

};

struct NExpr : public NNode {
    virtual void print() = 0;
    virtual Handle<TExpr> createT(
        std::map<std::string, int>& indexVarsMap,
        std::map<std::string, int>& tensorVarsMap) = 0;
};

struct NTensor : public NExpr {
    NTensor(string const& name, vector<string> const& idxs)
        : name(name), indices(idxs)
    {}

    string name;
    vector<string> indices;

    Handle<TExpr> createT(
        std::map<std::string, int>& indexVarsMap,
        std::map<std::string, int>& tensorVarsMap)
    {
        std::vector<int> whichIdxs;
        for(string const& index: indices) {
            if(indexVarsMap.count(index) == 0) {
                indexVarsMap[index] = indexVarsMap.size();
            }
            whichIdxs.push_back(indexVarsMap[index]);
        }

        if(tensorVarsMap.count(name) == 0) {
            tensorVarsMap[name] = tensorVarsMap.size();
        }

        return makeObject<TTensor>(tensorVarsMap[name], whichIdxs);
    }

    void print(){ std::cout << name << " "; }
};

//struct NConstant : public NExpr {
//    // TODO
//};

struct NBinOp : public NExpr {
    NBinOp(NExpr* lIn, NExpr* rIn)
        : lhs(lIn), rhs(rIn)
    {}

    NExprPtr lhs;
    NExprPtr rhs;

    void print(){ lhs->print(); rhs->print(); }
};

struct NMultOp : public NBinOp {
    NMultOp(NExpr* lIn, NExpr* rIn)
        : NBinOp(lIn, rIn)
    {}

    Handle<TExpr> createT(
        std::map<std::string, int>& indexVarsMap,
        std::map<std::string, int>& tensorVarsMap)
    {
        return makeObject<TMultOp>(
            lhs->createT(indexVarsMap, tensorVarsMap),
            rhs->createT(indexVarsMap, tensorVarsMap));
    }
};

struct NPlusOp : public NBinOp {
    NPlusOp(NExpr* lIn, NExpr* rIn)
        : NBinOp(lIn, rIn)
    {}

    Handle<TExpr> createT(
        std::map<std::string, int>& indexVarsMap,
        std::map<std::string, int>& tensorVarsMap)
    {
        return makeObject<TPlusOp>(
            lhs->createT(indexVarsMap, tensorVarsMap),
            rhs->createT(indexVarsMap, tensorVarsMap));
    }
};

struct NInput : public NNode {
    NInput(NTensor *tensorIn)
      : tensor(tensorIn)
    {}

    NTensorPtr tensor;
};

struct NOutput : public NNode {
    NOutput(string const& tIn)
        : tensorName(tIn)
    {}

    string tensorName;
};

struct NAssignment : public NNode {
    NAssignment(NTensor* tIn, NExpr* eIn)
        : lhs(tIn), rhs(eIn)
    {
        // TODO: Check that indices(lhs) subset indices(rhs)
    }

    Handle<TAssignment> createT() {
        std::map<std::string, int> indexVarsMap;
        std::map<std::string, int> tensorVarsMap;

        return createT(indexVarsMap, tensorVarsMap);
    }

    // this version is the same as createT except
    // indexVarsMap and tensorVarsMap are output too
    Handle<TAssignment> createT(
        std::map<std::string, int>& indexVarsMap,
        std::map<std::string, int>& tensorVarsMap)
    {
        // make sure they are empty maps
        indexVarsMap.clear();
        tensorVarsMap.clear();

        tensorVarsMap[lhs->name] = 0;

        // the free indices must come first, as in
        // they must be assigned to 0,1,...,numFree-1
        std::vector<int> lhsWhichIdxs;
        for(string const& index: lhs->indices) {
            if(indexVarsMap.count(index) == 0) {
                indexVarsMap[index] = indexVarsMap.size();
            }
            lhsWhichIdxs.push_back(indexVarsMap[index]);
        }

        Handle<TTensor> tleft = makeObject<TTensor>(0, lhsWhichIdxs);
        int numFree = indexVarsMap.size();
        Handle<TExpr> tright  = rhs->createT(indexVarsMap, tensorVarsMap);
        return makeObject<TAssignment>(numFree, indexVarsMap.size(), tleft, tright);
    }

    NTensorPtr lhs;
    NExprPtr rhs;
};

struct NProgram : public NNode {
    NProgram() {}

    std::vector<Handle<Computation>> compile(
        std::string const& dataset,
        std::map<string, Handle<Computation>> computations)
    {
        // check that each input in inputs is accounted for by computations
        for(auto input: inputs) {
            if(computations.count(input->tensor->name) == 0) {
                std::cout << "Error! Input " << input->tensor->name <<
                    " was not provided." << std::endl;
                return {};
            }
        }

        // for each assignment, create a join that will carry out the
        // assignment computation. Then (TODO) determine if doing an aggregation
        // is necessary and do it if so
        for(auto assignment: assignments) {
            std::string const& compName = assignment->lhs->name;

            std::map<std::string, int> indexVarsMap;
            std::map<std::string, int> tensorVarsMap;
            Handle<TAssignment> tAssignment = assignment->createT(
                    indexVarsMap,
                    tensorVarsMap);

            Handle<Computation> comp = nullptr;
            if(tAssignment->numExprLeafNodes == 1) {
                comp = makeObject<TacoProjection>(tAssignment);
            } else if(tAssignment->numExprLeafNodes == 2) {
                comp = makeObject<TacoJoin2>(tAssignment);
            } else if(tAssignment->numExprLeafNodes == 3) {
                comp = makeObject<TacoJoin3>(tAssignment);
            } else {
                // TODO 4,5 size joins too
                std::cout << "Error: not implemented yet!" << std::endl;
                return {};
            }

            // set the inputs in the correct position using tensorVarsMap
            for(auto p: tensorVarsMap) {
                if(p.second == 0) {
                    continue;
                }

                // the 0th index is the lhs, so subtract 1
                int const& position = p.second-1;
                std::string const& name = p.first;

                if(computations.count(name) == 0) {
                    std::cout << "Error! computation " << name <<
                        " was not found when compiling " <<
                        compName << std::endl;
                    return {};
                }
                if(!comp->setInput(position, computations[name])) {
                    std::cout << "OH no could not set input for computations of " <<
                        compName << std::endl;
                }
            }

            // TODO: determine if an aggregation is necessary
            if(true) {
                Handle<Computation> agg = makeObject<TacoAggregation>();
                agg->setInput(0, comp);
                comp = agg;
            }

            // It is ok if we overwrite another variable of the same name
            computations[compName] = comp;
        }

        // write out all of the output computations and return
        std::vector<Handle<Computation>> out;
        for(auto output: outputs) {
            std::string const& outputName = output->tensorName;
            if(computations.count(outputName) == 0) {
                std::cout << "Warning! Output of name " << outputName <<
                    " was not found" << std::endl;
                // ok to continue, though
            }
            out.push_back(makeObject<TacoWriter>(dataset, outputName));
            out.back()->setInput(0, computations[outputName]);
        }

        return out;
    }



    // these add functions take ownership of the pointer!
    void addAssignment(NAssignment* a) {
        // TODO: check that each tensor is available
        // TODO: check that each tensor is of the right order
        assignments.emplace_back(a);
        std::cout << assignments.size() << std::endl;
    }
    void addInput(NInput* i) {
        // TODO: check that tensor of same name has not already been added
        inputs.emplace_back(i);
    }
    void addOutput(NOutput* o) {
        // TODO check that the tensor name is present
        outputs.emplace_back(o);
    }

    std::vector<NAssignmentPtr> assignments;
    std::vector<NInputPtr> inputs;
    std::vector<NOutputPtr> outputs;
};

#endif
