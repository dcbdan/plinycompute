#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

/*************************************************/
/** HERE WE DEFINE ALL OF THE STRUCTS THAT ARE **/
/** PASSED AROUND BY THE PARSER                **/
/*************************************************/

#include <memory>
#include <string>
#include <vector>
#include <iostream>

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
};

struct NTensor : public NExpr {
    NTensor(string const& name, vector<string> const& idxs)
        : name(name), indices(idxs)
    {}

    string name;
    vector<string> indices;

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
};

struct NPlusOp : public NBinOp {
    NPlusOp(NExpr* lIn, NExpr* rIn)
        : NBinOp(lIn, rIn)
    {}
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
        // TODO: Check that inices(lhs) subset indices(rhs)
    }

    NTensorPtr lhs;
    NExprPtr rhs;
};

struct NProgram : public NNode {
    NProgram() {}

    // these add functions take ownership of the pointer!
    void addAssignment(NAssignment* a) {
        // TODO: check that each tensor is available
        // TODO: check that each tensor is of the right order
        assignments.emplace_back(a);
    }
    void addInput(NInput* i) {
        // TODO: check that tensor of same name has not already been added
        std::cout << "Adding input\n";
        inputs.emplace_back(i);
    }
    void addOutput(NOutput* o) {
        // TODO check that the tensor name is present
        std::cout << "Adding output\n";
        outputs.emplace_back(o);
    }

    std::vector<NAssignmentPtr> assignments;
    std::vector<NInputPtr> inputs;
    std::vector<NOutputPtr> outputs;
};

#endif
