#pragma once

#include "TacoTensorBlock.h"
#include "TacoJoinAux.h"

#include "TAssignment.h"

#include "LambdaCreationFunctions.h"
#include "JoinComp.h"
#include "TacoModuleMap.h"

using namespace pdb;

class TacoJoin2: public JoinComp<
    TacoJoin2,
    TacoTensorBlock,
    TacoTensorBlock,
    TacoTensorBlock> {
public:
    ENABLE_DEEP_COPY

    TacoJoin2() = default;

    TacoJoin2(Handle<TAssignment> assignmentIn)
        : joinAux(assignmentIn)
    {
    }

    Lambda<bool>
    getKeySelection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2)
    {
        if(joinAux.myAssignment->whichInputL.size() == 0) {
            return makeLambda(in1, in2, [](
                Handle<TacoTensorBlockMeta>&,
                Handle<TacoTensorBlockMeta>&)
                {
                    return true;
                });
        }

        return joinAux.getKeySelection({nullptr, &in1, &in2});
    }

    Lambda<Handle<TacoTensorBlockMeta>>
    getKeyProjection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2)
    {
        return makeLambda(in1, in2, [this](
            Handle<TacoTensorBlockMeta>& in1,
            Handle<TacoTensorBlockMeta>& in2)
        {
            return this->joinAux.getKeyProjection({nullptr, &in1, &in2});
        });
    }

    Lambda<Handle<TacoTensor>>
    getValueProjection(
        Handle<TacoTensor> in1,
        Handle<TacoTensor> in2)
    {
        return makeLambda(in1, in2, [this](
            Handle<TacoTensor>& in1,
            Handle<TacoTensor>& in2)
        {
            return this->joinAux.getValueProjection({nullptr, in1, in2});
        });
    }
private:
    TacoJoinAux joinAux;
};

class TacoJoin3: public JoinComp<
    TacoJoin3,
    TacoTensorBlock,
    TacoTensorBlock,
    TacoTensorBlock,
    TacoTensorBlock> {
public:
    ENABLE_DEEP_COPY

    TacoJoin3() = default;

    TacoJoin3(Handle<TAssignment> assignmentIn)
        : joinAux(assignmentIn)
    {}

    Lambda<bool>
    getKeySelection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2,
        Handle<TacoTensorBlockMeta> in3)
    {
        if(joinAux.myAssignment->whichInputL.size() == 0) {
            return makeLambda(in1, in2, in3, [](
                Handle<TacoTensorBlockMeta>&,
                Handle<TacoTensorBlockMeta>&,
                Handle<TacoTensorBlockMeta>&)
                {
                    return true;
                });
        }

        // TODO std::move? and in other tacojoin#
        return joinAux.getKeySelection({nullptr, &in1, &in2, &in3});
    }

    Lambda<Handle<TacoTensorBlockMeta>>
    getKeyProjection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2,
        Handle<TacoTensorBlockMeta> in3)
    {
        return makeLambda(in1, in2, in3, [this](
            Handle<TacoTensorBlockMeta>& in1,
            Handle<TacoTensorBlockMeta>& in2,
            Handle<TacoTensorBlockMeta>& in3)
        {
            return this->joinAux.getKeyProjection({nullptr, &in1, &in2, &in3});
        });
    }

    Lambda<Handle<TacoTensor>>
    getValueProjection(
        Handle<TacoTensor> in1,
        Handle<TacoTensor> in2,
        Handle<TacoTensor> in3)
    {
        return makeLambda(in1, in2, in3, [this](
            Handle<TacoTensor>& in1,
            Handle<TacoTensor>& in2,
            Handle<TacoTensor>& in3)
        {
            return this->joinAux.getValueProjection({nullptr, in1, in2, in3});
        });
    }
private:
    TacoJoinAux joinAux;
};


