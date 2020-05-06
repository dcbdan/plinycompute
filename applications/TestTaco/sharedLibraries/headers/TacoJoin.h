#pragma once

#include "TacoTensorBlock.h"

#include "LambdaCreationFunctions.h"
#include "JoinComp.h"
#include "TacoModuleMap.h"

using namespace pdb;

class TacoJoin: public JoinComp<
    TacoJoin,
    TacoTensorBlock,
    TacoTensorBlock,
    TacoTensorBlock> {
public:
    ENABLE_DEEP_COPY

    TacoJoin() = default;

    // TODO: what about larger join?
    static Lambda<bool>
    getKeySelection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2)
    {
        return getFixed(in1, fixedL) == getLambdaRight(in2, fixedR);
    }

    static Lambda<Handle<TacoTensorBlockMeta>>
    getKeyProjection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2)
    {
        // capturing this should be ok
        return makeLambda(in1, in2, [this](
            Handle<TacoTensorBlockMeta>& in1,
            Handle<TacoTensorBlockMeta>& in2)
        {
            using Meta = TacoTensorBlockMeta;
            //std::vector<uint32_t> idxs = {in1->access(0), in2->access(1)};
            //Handle<Meta> out = makeObject<Meta>(idxs);
            Handle<Meta> out = makeObject<Meta>(
                std::vector<uint32_t>({in1->access(0), in2->access(1)}));

            return out;
        });
    }

    static Lambda<Handle<TacoTensor>>
    getValueProjection(
        Handle<TacoTensor> in1,
        Handle<TacoTensor> in2)
    {
        return makeLambda(in1, in2, [](
        Handle<TacoTensor>& in1,
        Handle<TacoTensor>& in2)
        {
            // TODO make output type be the least dense viable output type
            //
            // create the output TacoTensor. The output is a
            // dense matrix
            Handle<TacoTensor> out = makeObject<TacoTensor>(
                in1->getDatatype(),
                std::vector<int>({in1->getDimension(0), in2->getDimension(1)}),
                taco::Format({taco::dense, taco::dense}));

            // create the assignment statement
            taco::TensorVar C = out->getTensorVar();
            taco::TensorVar A = in1->getTensorVar();
            taco::TensorVar B = in2->getTensorVar();

            taco::IndexVar i,j,k;

            taco::Assignment assignment = C(i,k) = A(i,j) * B(j,k);

            // get the function to compute out
            TacoModuleMap m;
            void* function = m[assignment];

            // run the computation, this will allocate memory in
            // out as necessary, not that that is special for a dense
            // matrix. It should also work regardless of the
            // sparsity structure of in1/in2
            TacoTensor::callKernel(function, {out, in1, in2});

            return out;
        });
    }
private:
    // these are constant members that define the computation that the join runs
    pdb::Vector<int> fixedL;
    pdb::Vector<int> fixedR;

    // the TacoAssignment object stores the computation that this
    // join runs for each set of matching blocks
    TacoAssignment myAssignment;
private:
    Lambda<Vector<uint32_t> > getFixed(
        Handle<TacoTensorBlockMeta> in,
        pdb::Vector<int>& fixed)
    {
        // fixed is caught by reference in the C++ lambda.
        // this shouldn't cause any problems
        return makeLambda(in1, [&fixed](Handle<TacoTensorBlockMeta>& in) {
            size_t size = fixed.size();
            pdb::Vector<uint32_t> out(size);
            for(size_t idx = 0; idx != size; ++idx) {
                out.push_back(in->access(fixed[idx]));
            }
            return out;
        });
    }
};
