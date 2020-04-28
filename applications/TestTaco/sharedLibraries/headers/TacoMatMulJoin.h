#pragma once

#include "TacoTensorBlock.h"

#include "LambdaCreationFunctions.h"
#include "JoinComp.h"
#include "TacoModuleMap.h"

using namespace pdb;

// We assume that the two inputs each have
// block-order of 2--just like matrices.
// TODO: create a TacoContractJoin/TacoContractAggregate
class TacoMatMulJoin: public JoinComp<
    TacoMatMulJoin,
    TacoTensorBlock,
    TacoTensorBlock,
    TacoTensorBlock> {
public:
    ENABLE_DEEP_COPY

    TacoMatMulJoin() = default;

    static Lambda<bool>
    getKeySelection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2)
    {
        // TODO: it doesn't allow the add arguments at the end!
        //       insstead of make(in1, access, 1), doing
        //       make(in1, access<1>) with a templated on int method
        return makeLambdaFromMethod(in1, access<1>) ==
               makeLambdaFromMethod(in2, access<0>);
    }

    static Lambda<Handle<TacoTensorBlockMeta>>
    getKeyProjection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2)
    {
        return makeLambda(in1, in2, [](
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
};
