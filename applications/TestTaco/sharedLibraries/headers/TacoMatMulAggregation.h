#pragma once

#include "TacoTensorBlock.h"

#include "LambdaCreationFunctions.h"
#include "AggregateComp.h"

using namespace pdb;

class TacoMatMulAggregation : public AggregateComp<
    TacoMatMulAggregation,
    TacoTensorBlock,
    TacoTensorBlock,
    TacoTensorBlockMeta,
    TacoTensor>
{
public:
    ENABLE_DEEP_COPY

    TacoMatMulAggregation() = default;

    static Lambda<TacoTensorBlockMeta>
    getKeyProjectionWithInputKey(Handle<TacoTensorBlockMeta> aggMe) {
        return makeLambdaFromSelf(aggMe);
    }

    static Lambda<TacoTensor>
    getValueProjection(Handle<TacoTensorBlock> aggMe) {
        return makeLambdaFromMethod(aggMe, getValueRef);
    }
};
