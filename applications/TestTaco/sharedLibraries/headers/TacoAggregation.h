#pragma once

#include "TacoTensorBlock.h"

#include "LambdaCreationFunctions.h"
#include "AggregateComp.h"

using namespace pdb;

// This just sums all values with the same key
class TacoAggregation : public AggregateComp<
    TacoAggregation,
    TacoTensorBlock,
    TacoTensorBlock,
    TacoTensorBlockMeta,
    TacoTensor>
{
public:
    ENABLE_DEEP_COPY

    TacoAggregation() = default;

    static Lambda<TacoTensorBlockMeta>
    getKeyProjectionWithInputKey(Handle<TacoTensorBlockMeta> aggMe) {
        return makeLambdaFromSelf(aggMe);
    }

    static Lambda<TacoTensor>
    getValueProjection(Handle<TacoTensorBlock> aggMe) {
        return makeLambdaFromMethod(aggMe, getValueRef);
    }
};
