#pragma once

#include "TacoTensor.h"
#include "TraMeta.h"

class TraTensor : public pdb::Object {
public:
  TraTensor() = default;

  ENABLE_DEEP_COPY

  TraTensor(
    Handle<TacoTensor> data,
    Handle<TraMeta> meta)
    : data(data), meta(meta)
  {}

  Handle<TraMeta>& getKey() {
    return meta;
  }

  TraMeta& getKeyRef() {
    return *meta;
  }

  Handle<TacoTensor>& getValue() {
    return data;
  }

  TacoTensor& getValueRef() {
    return *data;
  }

  Handle<TacoTensor> data;
  Handle<TraMeta> meta;
};
