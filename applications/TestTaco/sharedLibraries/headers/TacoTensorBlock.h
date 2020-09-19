#pragma once

#include "TacoTensor.h"
#include "TacoTensorBlockMeta.h"

class TacoTensorBlock : public pdb::Object {
public:

    TacoTensorBlock() = default;

    ENABLE_DEEP_COPY

    // TODO: make the constructor so that TacoTensorBlock
    //       behaves more "object like"?
    TacoTensorBlock(
        Handle<TacoTensor> data,
        Handle<TacoTensorBlockMeta> metaData)
      : data(data), metaData(metaData) {
    }

    Handle<TacoTensorBlockMeta>& getKey() {
        return metaData;
    }

    TacoTensorBlockMeta& getKeyRef() {
        return *metaData;
    }

    Handle<TacoTensor>& getValue() {
        return data;
    }

    TacoTensor& getValueRef() {
        return *data;
    }
public:
    Handle<TacoTensor> data;
    Handle<TacoTensorBlockMeta> metaData;
};
