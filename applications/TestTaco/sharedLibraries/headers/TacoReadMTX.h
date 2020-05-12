#pragma once

#include "TacoTensorBlock.h"
#include "LambdaCreationFunctions.h"
#include "SelectionComp.h"

#include <string>

#include "BlockOffsetter.h"

#include "taco/tensor.h"
#include "taco/format.h"
#include "taco/error.h"
#include "taco/util/strings.h"
#include "taco/util/timers.h"
#include "taco/util/files.h"

namespace tacomod {

using namespace taco;

// These functions are basically a pastiche of the functions in
// file_io_mtx.cpp in taco, with the exception of
// 1) adding a BlockOffsetter is taken as input.
//    This is used to go from overall coordinate to local block coordinate
// 2) the format is set by depending on the matrix market header
//    If matrix market coordinates, use CSR o.w. use all dense.
TensorBase readDenseOffset(
    std::istream& stream,
    bool symm,
    BlockOffsetter const& offsetter);
TensorBase readSparseOffset(
    std::istream& stream,
    bool symm,
    BlockOffsetter const& offsetter);
TensorBase readMTXOffset(
    std::istream& stream,
    BlockOffsetter const& offsetter)
{
    string line;
    if (!std::getline(stream, line)) {
        return TensorBase();
    }

    // Read Header
    std::stringstream lineStream(line);
    string head, type, formats, field, symmetry;
    lineStream >> head >> type >> formats >> field >> symmetry;
    taco_uassert(head=="%%MatrixMarket") << "Unknown header of MatrixMarket";
    // type = [matrix tensor]
    taco_uassert((type=="matrix") || (type=="tensor"))
                                 << "Unknown type of MatrixMarket";
    // formats = [coordinate array]
    // field = [real integer complex pattern]
    taco_uassert(field=="real")          << "MatrixMarket field not available";
    // symmetry = [general symmetric skew-symmetric Hermitian]
    taco_uassert((symmetry=="general") || (symmetry=="symmetric"))
                                 << "MatrixMarket symmetry not available";

    bool symm = (symmetry=="symmetric");

    TensorBase tensor;
    if (formats=="coordinate")
        tensor = readSparseOffset(stream,symm,offsetter);
    else if (formats=="array")
        tensor = readDenseOffset(stream,symm,offsetter);
    else
        taco_uerror << "MatrixMarket format not available";

    tensor.pack();

    return tensor;
}

TensorBase readDenseOffset(
    std::istream& stream,
    bool symm,
    BlockOffsetter const& offsetter)
{
    string line;
    std::getline(stream,line);

    // Skip comments at the top of the file
    string token;
    do {
        std::stringstream lineStream(line);
        lineStream >> token;
        if (token[0] != '%') {
            break;
        }
    } while (std::getline(stream, line));

    // The first non-comment line is the header with dimension sizes
    // But this information is already in the offsetter, and this
    // needs the block dimensions
    vector<int> blockDimensions = offsetter.getDimensionOfBlock();
    vector<int> dimensions;
    char* linePtr = (char*)line.data();
    while (size_t dimension = strtoul(linePtr, &linePtr, 10)) {
        taco_uassert(dimension <= INT_MAX) << "Dimension exceeds INT_MAX";
        dimensions.push_back(static_cast<int>(dimension));
    }

    if (symm)
        taco_uassert(dimensions.size()==2) << "Symmetry only available for matrix";

    auto blockSize = std::accumulate(
        begin(blockDimensions),
        end(blockDimensions),
        1,
        std::multiplies<double>());

    // Create tensor
    TensorBase tensor(
        type<double>(),
        blockDimensions,
        taco::Format(std::vector<taco::ModeFormatPack>(blockDimensions.size(), taco::dense)));

    tensor.reserve(blockSize);

    int n = 0;
    std::vector<int> coord;
    while (std::getline(stream, line)) {
        linePtr = (char*)line.data();
        double val = strtod(linePtr, &linePtr);

        coord.clear();
        auto index=n;
        for (size_t mode = 0; mode < dimensions.size()-1; mode++) {
            coord.push_back(index%dimensions[mode]);
            index=index/dimensions[mode];
        }
        coord.push_back(index);

        // If this coordinate is in the block, add it
        if(offsetter.inBlock(coord)) {
            tensor.insert(offsetter.getCoordInBlock(coord), val);
        }

        n += 1;
    }

    return tensor;
}

TensorBase readSparseOffset(
    std::istream& stream,
    bool symm,
    BlockOffsetter const& offsetter)
{
    string line;
    std::getline(stream,line);

    // Skip comments at the top of the file
    string token;
    do {
      std::stringstream lineStream(line);
      lineStream >> token;
      if (token[0] != '%') {
        break;
      }
    } while (std::getline(stream, line));

    // The first non-comment line is the header with dimensions.
    // But this information is already in the offsetter, and this
    // needs the block dimensions, not the overall dimensions
    vector<int> blockDimensions = offsetter.getDimensionOfBlock();
    size_t nnz;
    char* linePtr = (char*)line.data();
    while (size_t dimension = strtoul(linePtr, &linePtr, 10)) {
        taco_uassert(dimension <= INT_MAX) << "Dimension exceeds INT_MAX";
        // the last "dimension" is actually the number of nonzeros for the
        // whole matrix
        nnz = static_cast<int>(dimension);
    }

    if (symm)
        taco_uassert(blockDimensions.size()==2) << "Symmetry only available for matrix";

    // Create tensor with all dense indices except the last..
    // For a matrix, this is CSR
    std::vector<taco::ModeFormatPack> format(blockDimensions.size()-1, taco::dense);
    format.push_back(taco::sparse);
    TensorBase tensor(
        type<double>(),
        blockDimensions,
        taco::Format(format));

    // Reserve space. It isn't known how much
    // space needs to be reserved for the particular block, so
    // as a guess, twice the average number of elements
    // per block
    if (symm)
      tensor.reserve(4*nnz / offsetter.getNumBlocks());
    else
      tensor.reserve(2*nnz / offsetter.getNumBlocks());

    std::vector<int> coord;
    while (std::getline(stream, line)) {
        coord.clear();
        linePtr = (char*)line.data();
        for (size_t i=0; i < blockDimensions.size(); i++) {
            long index = strtol(linePtr, &linePtr, 10);
            taco_uassert(index <= INT_MAX) << "Index exceeds INT_MAX";
            coord.push_back(static_cast<int>(index) - 1); // mtx is 1 indexed
        }
        double val = strtod(linePtr, &linePtr);

        // If this coordinate is in the block, add it
        if(offsetter.inBlock(coord)) {
            tensor.insert(offsetter.getCoordInBlock(coord), val);
        }
        // If this is a symmetric matrix, check if the reverse
        // coordinate needs to be added
        if(symm && coord.front() != coord.back()) {
            std::reverse(coord.begin(), coord.end());
            if(offsetter.inBlock(coord)) {
                tensor.insert(offsetter.getCoordInBlock(coord), val);
            }
        }
    }

    return tensor;
}

} // tacomod namespace

using namespace pdb;

// Read a matrix market file.. Each block has to be handled
class TacoReadMTX: public SelectionComp<TacoTensorBlock, TacoTensorBlock> {
public:
    ENABLE_DEEP_COPY

    TacoReadMTX() = default;

    // there could also be a constructor
    // that reads in the dimensions from
    // the file
    TacoReadMTX(
        std::string filenameIn,
        std::vector<uint32_t> dimIn,
        std::vector<uint32_t> blockDimIn)
    {
        filename.reserve(filenameIn.size());
        for(auto c: filenameIn) {
            filename.push_back(c);
        }

        copyToVector(dim, dimIn);
        copyToVector(blockDim, blockDimIn);
    }

    Lambda<bool> getSelection(Handle<TacoTensorBlock> in) override {
        return makeLambda(in, [](Handle<TacoTensorBlock>& in){ return true; });
    }

    Lambda<Handle<TacoTensorBlock>>
    getProjection(Handle<TacoTensorBlock> in) override {
        return makeLambda(in, [this](Handle<TacoTensorBlock>& block)
        {
            Handle<TacoTensorBlock> out = makeObject<TacoTensorBlock>(
                this->readMTX(block->getKey()),
                block->getKey());
            return out;
        });
    }

private:
    Handle<TacoTensor> readMTX(Handle<TacoTensorBlockMeta>& key) {
        std::string filenameStr;
        filenameStr.reserve(filename.size());
        for(int i = 0; i != filename.size(); ++i) {
            filenameStr.push_back(filename[i]);
        }

        std::fstream file;
        taco::util::openStream(file, filenameStr, fstream::in);

        std::vector<uint32_t> dimS, blockDimS, blockS;
        copyToStd(dimS, dim);
        copyToStd(blockDimS, blockDim);
        copyToStd(blockS, key->getBlock());

        BlockOffsetter offsetter(dimS, blockDimS, blockS);
        taco::TensorBase tensorBase = tacomod::readMTXOffset(file, offsetter);

        file.close();

        // This is inefficient, but now we copy it into pdb
        Handle<TacoTensor> out = makeObject<TacoTensor>(tensorBase);

        return out;
    }

    Vector<char> filename;

    Vector<uint32_t> dim;
    Vector<uint32_t> blockDim;

private:
    template<typename T, typename U>
    static void copyToVector(pdb::Vector<T>& to, std::vector<U> const& from) {
        to.clear();
        to.reserve(from.size());
        for(U const& u: from) {
            to.push_back(u);
        }
    }

    template<typename T, typename U>
    static void copyToStd(std::vector<U>& to, Vector<T> const& from) {
        to.clear();
        to.reserve(from.size());
        for(int i = 0; i != from.size(); ++i) {
            to.push_back(from[i]);
        }
    }
};
