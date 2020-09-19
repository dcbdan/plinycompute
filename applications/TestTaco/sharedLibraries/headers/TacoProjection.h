#pragma once

#include "TacoTensorBlock.h"
#include "TacoJoinAux.h"

#include "TAssignment.h"

#include "LambdaCreationFunctions.h"
#include "SelectionComp.h"
#include "TacoModuleMap.h"

using namespace pdb;

class TacoProjection: public SelectionComp<TacoTensorBlock, TacoTensorBlock> {
public:
    ENABLE_DEEP_COPY

    TacoProjection() = default;

    TacoProjection(Handle<TAssignment> assignmentIn)
        : aux(assignmentIn)
    {}

    Lambda<bool> getSelection(Handle<TacoTensorBlock> in) override {
        return makeLambda(in, [](Handle<TacoTensorBlock>& in){ return true; });
    }

    Lambda<Handle<TacoTensorBlock>> getProjection(Handle<TacoTensorBlock> in) override{
        return makeLambda(in, [this](Handle<TacoTensorBlock>& in)
        {
            Handle<TacoTensor> value = this->aux.getValueProjection(
                {nullptr, in->getValue()});
            Handle<TacoTensorBlockMeta> key = this->aux.getKeyProjection(
                {nullptr, &(in->getKey())});
            Handle<TacoTensorBlock> out = makeObject<TacoTensorBlock>(value, key);
            return out;
        });
    }
private:
    TacoJoinAux aux;
};
