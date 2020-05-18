#pragma once

#include <Object.h>
#include <Handle.h>
#include "FFMatrixMeta.h"
#include "FFMatrixData.h"

namespace pdb {

// the sub namespace
namespace ff {

/**
 * This represents a block in a large matrix distributed matrix.
 * For example if the large matrix has the size of 10000x10000 and is split into 4 blocks of size 2500x2500
 * Then we would have the following blocks in the system
 *
 * |metaData.colID|metaData.rowID|data.numRows|data.numCols| data.block |
 * |       0      |       1      |    25k     |    25k     | 25k * 25k  |
 * |       1      |       1      |    25k     |    25k     | 25k * 25k  |
 * |       0      |       0      |    25k     |    25k     | 25k * 25k  |
 * |       1      |       0      |    25k     |    25k     | 25k * 25k  |
 */
class FFMatrixBlock : public pdb::Object {
 public:

  /**
   * The default constructor
   */
  FFMatrixBlock() = default;

  /**
   * The constructor for a block size
   * @param rowID - the value we want to initialize the row id to
   * @param colID - the value we want to initialize the col id to
   * @param numRows - the number of rows the block has
   * @param numCols - the number of columns the block has
   */
  FFMatrixBlock(uint32_t rowID, uint32_t colID, uint32_t numRows, uint32_t numCols) {
    metaData = makeObject<FFMatrixMeta>(rowID, colID),
        data = makeObject<FFMatrixData>(numRows, numCols);
  }

  ENABLE_DEEP_COPY

  /**
   * The metadata of the matrix
   */
  Handle<FFMatrixMeta> metaData;

  /**
   * The data of the matrix
   */
  Handle<FFMatrixData> data;

  /**
   *
   * @return
   */
  Handle<FFMatrixMeta>& getKey() {
    return metaData;
  }

  /**
   *
   * @return
   */
  FFMatrixMeta& getKeyRef(){
    return *metaData;
  }

  /**
   *
   * @return
   */
  Handle<FFMatrixData>& getValue() {
    return data;
  }

  FFMatrixData& getValueRef() {
    return *data;
  }

  uint32_t getRowID() {
    return metaData->rowID;
  }

  uint32_t getColID() {
    return metaData->colID;
  }

  uint32_t getNumRows() {
    return data->numRows;
  }

  uint32_t getNumCols() {
    return data->numCols;
  }
};


}

}