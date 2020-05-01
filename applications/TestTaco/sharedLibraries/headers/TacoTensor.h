#pragma once

#include <cstring>
#include <type_traits>
#include <vector>

#include <Object.h>
#include <PDBVector.h>
#include <Nothing.h>

#include <TacoModuleMap.h>

#include "Arr.h"

#include <taco/format.h>
#include <taco/type.h>
#include <taco/tensor.h>
#include <taco/taco_tensor_t.h>

using namespace pdb;

class TacoTensor : public Object {
public:

    ENABLE_DEEP_COPY

    TacoTensor() {}

    // construct a taco tensor object
    TacoTensor(
        taco::Datatype componentType,
        std::vector<int> dimensions,
        taco::Format format);

    TacoTensor(taco::TensorBase& tensorToCopy) {
        taco::Datatype datatypeFromTaco = tensorToCopy.getComponentType();

        taco_tensor_t* fromTaco = tensorToCopy.getTacoTensorT();

        // for some reason the taco_tensor_t.csize produced from
        // tensorToCopy.getTacoTensorT is not always correct.
        // So this is to avoid a segfault...
        fromTaco->csize = datatypeFromTaco.getNumBytes();

        // set *this datatype
        datatype = datatypeFromTaco.getKind();

        // set order to 0 for copyFrom to know how big *this is
        order = 0;
        copyFrom(fromTaco);
    }

    // this makes a deep copy from other to this TacoTensor
    TacoTensor(taco::Datatype componentType, taco_tensor_t* other);

//    // this makes a deep copy from other to this TacoTensor
//    TacoTensor& operator=(TacoTensor& other);
//
//    // this make a deep copy from other to this TacoTensor
//    // TODO: why isnt' this TacoTensor const& ?
//    TacoTensor(TacoTensor& other);

    // TODO this may not be a good idea
    // taco::TensorBase shallowCopyToTaco() const;

    taco::TensorBase copyToTaco() const;

    taco::Datatype getDatatype() const {
        return taco::Datatype((taco::Datatype::Kind(datatype)));
    }

    std::vector<int> getDimensions() const {
        std::vector<int> ret;
        ret.reserve(order);
        for(int32_t i = 0; i != order; ++i) {
            ret.push_back(dimensions[i]);
        }
        return ret;
    }

    int getDimension(int mode) {
        return dimensions[mode]; // TODO: need to use modeOrdering here? TODO test modeOrdering!
    }

    taco::Format getFormat() const {
        std::vector<taco::ModeFormatPack> modeFormatOut;
        std::vector<int> modeOrderingOut;
        modeFormatOut.reserve(order);
        modeOrderingOut.reserve(order);
        for(int32_t i = 0; i != order; ++i) {
            modeFormatOut.emplace_back(
                modeTypes[i] == taco_mode_t::taco_mode_dense ?
                taco::dense                                  :
                taco::sparse);
            modeOrderingOut.push_back(modeOrdering[i]);
        }
        return taco::Format(modeFormatOut, modeOrderingOut);
    }

    taco::TensorVar getTensorVar() const {
        taco::Type type(getDatatype(), std::vector<taco::Dimension>(order));
        return taco::TensorVar(type, getFormat());
    }

    void printDiagnostics() {
        auto printH = [](Handle<Arr>& h, std::string tab) {
            if(h.isNullPtr()) {
                std::cout  << tab << "nullptr" << std::endl;
            } else {
                std::cout << tab << h->size << std::endl;
            }
        };

        for(int i = 0; i != order; ++i) {
            std::cout << "mode: " << i << std::endl;
            if(modeTypes[i] == taco_mode_t::taco_mode_dense) {
                std::cout << "\tdense"  << std::endl;
                printH(inds[i][0], "\t");
            } else {
                std::cout << "\tsparse" <<   std::endl;
                printH(inds[i][0], "\t");
                printH(inds[i][1], "\t");
            }
        }

        printH(vals, "");
    }

    TacoTensor& operator+(TacoTensor& other) {
        // TODO: we don't want to do
        //         A(dense, sparse) += B(dense, dense)
        //       or worse
        //         A(dense, sparse) += B(sparse, dense)
        //       The output mode of an axis
        //       should not go from dense to sparse.
        //       sparse->sparse, dense->dense and sparse->dense
        //       are OK, but dense->sparse will just end up with a
        //       a sparse axis storing everything..

        // TODO: check whether or not A(is) += B(is) can be used.
        //       With
        //       https://github.com/tensor-compiler/taco commit
        //       4104c9ca99d2cfc1382fe77670fef3a7fb505dec , the
        //       compiled code gave errors.

        TacoTensor out(
            getDatatype(),
            getDimensions(),
            getFormat());

        // get the kernel
        taco::TensorVar Aout = out.getTensorVar();
        taco::TensorVar Ain = this->getTensorVar();
        taco::TensorVar B = other.getTensorVar();

        // Set the name for debugging purposes  TODO
        Aout.setName("Aout");
        Ain.setName("Ain");
        B.setName("B");

        std::vector<taco::IndexVar> is(order);
        taco::Assignment assignment = Aout(is) = Ain(is) + B(is);
        pdb::TacoModuleMap m;
        void* kernel = m[assignment];

        if(kernel == nullptr) {
            std::cout   << "NOOOOOOOOOOOOOO\n"; // TODO
        }

        // call the kernel
        std::vector<taco_tensor_t*> ts({
            out.init_temp_c_ptr(),
            init_temp_c_ptr(),
            other.init_temp_c_ptr()});

        TacoTensor::callKernel(kernel, ts);

        // the pointers have to be deinited
        out.deinit_temp_c_ptr(ts[0]);
        deinit_temp_c_ptr(ts[1]);
        other.deinit_temp_c_ptr(ts[2]);

        // now, copy over..
        *this = out;

        return *this;
    }

    // this function is defined inside TacoTensor so it can call init_temp_c_ptr and such..
    // whenever it is done with temp taco_tensor_t objects, it needs to free the space
    // to store the taco_tensor_t and the pointers that point to data managed by TacoTensor
    static void callKernel(void* function, std::vector<Handle<TacoTensor>> tensors);

    // it could be the case that taco_tensor_t's vals and indices fields
    // could be rewritten by calls malloc and realloc...
    // To handle this, a custom malloc and realloc code will be injected into
    // taco generated kernels.
    // Whenever the custom calloc is called, makeObjectWithExtraStorage is used to

    // Get the number of bytes managed by this object
    size_t getNumBytes(); // TODO

    // What about a function that compresses this object while still having the same format?
    // When a taco kernel is called, it will (1) probably allocate more memory than necessary
    // and (2) not check if it has unnecessary 0s in the vals array.
    // There is a pack function in taco, but it packs coordinates arrays, not arbitrary shapes.
    // So if you want to pack an array, you have to create a coordinate list..
    // See Between taco::TensorBase::pack and taco::pack.
    // TansorBase::pack compiles the packing code whereas pack can be called directly.
    // Having the most condensed shape should for a format _should_ then  be a matter of
    // not adding 0's to a coordinate array and calling pack.

private:
    // this could not call copyFrom on itself..
    // copyFrom(this->c_ptr()) would be incorrect!
    size_t copyFrom(taco_tensor_t* other);

    // this taco_tensor_t returned is only valid while
    // all the handles are on the current allocation block..
    // deinit_temp_c_ptr needs to be called or a memory leak
    // will occur
    taco_tensor_t* init_temp_c_ptr();
    void deinit_temp_c_ptr(taco_tensor_t*);

    // This should be private so that it can only be called from a TacoTensor
    // object
    static void callKernel(void* function, std::vector<taco_tensor_t*> ts);
private:
    using mode_t = std::underlying_type<taco_mode_t>::type;
    using kind_t = std::underlying_type<taco::Datatype::Kind>::type;

    kind_t                            datatype;     // store the underlying datatype
    int32_t                           order;        // tensor order (number of modes)
    Vector<int32_t>                   dimensions;   // tensor dimensions
    int32_t                           csize;        // component size
    Vector<int32_t>                   modeOrdering; // mode storage ordering
    // TODO: can I do Vector<taco_mode_t>?
    Vector<mode_t>                    modeTypes;    // mode storage types

    // ind1[i][j] is the start of an array of values of some size
    Vector<Vector<Handle<Arr>>> inds;
    // Store info needed to construct a taco_tensor_t indices field.
    // indices field is of type uint8_t*** where:
    //   indices[j] goes from j=0,1,...,order-1
    //   for sparse mode j,
    //      indices[j][k] goes from k=0 (pos) to 1(crd)
    //  for dense mode 0,
    //      only indices[j][0] is used
    // And inds[i][j] stores some amount of data

    Handle<Arr>                   vals;         // the values array
};

TacoTensor::TacoTensor(
    taco::Datatype componentType,
    std::vector<int> dimensionsIn,
    taco::Format format) {
    // After this call, all members should be set except sparse inds
    // and vals[i][j] which should be nullptrs.

    order = format.getOrder();
    datatype = componentType.getKind();
    csize = componentType.getNumBits();

    dimensions = Vector<int32_t>(order, order);
    int32_t* dimensions_ = dimensions.c_ptr();

    modeOrdering = Vector<int32_t>(order, order);
    int32_t* modeOrdering_ = modeOrdering.c_ptr();

    modeTypes = Vector<mode_t>(order, order);
    mode_t* modeTypes_ = modeTypes.c_ptr();

    inds = Vector<Vector<Handle<Arr>>>(order, order);

    for(int i = 0; i != order; ++i) {
        dimensions_[i] = dimensionsIn[i];
        modeOrdering_[i] = format.getModeOrdering()[i];
        inds[i] = Vector<Handle<Arr>>(2,2);
        auto modeType  = format.getModeFormats()[i];
        if (modeType.getName() == taco::Dense.getName()) {
            modeTypes_[i] = taco_mode_t::taco_mode_dense;
            inds[i] = Vector<Handle<Arr>>(1,1);
            inds[i][0] = makeObjectWithExtraStorage<Arr>(sizeof(int));
            inds[i][0]->size = sizeof(int);
            ((int*)inds[i][0]->data)[0] = dimensionsIn[i];
        } else if (modeType.getName() == taco::Sparse.getName()) {
            modeTypes_[i] = taco_mode_t::taco_mode_sparse;
            inds[i] = Vector<Handle<Arr>>(2,2);
        } else {
            // TODO: throw error
        }
    }
}

TacoTensor::TacoTensor(taco::Datatype componentType, taco_tensor_t* other) {
    // set order to 0 so copyFrom knows how big *this is
    order = 0;

    // set datatype, since copyFrom can't set that
    // (taco_tensor_t only knows size of the datatype)
    datatype = componentType.getKind();

    copyFrom(other);
}

//// TODO: this is kosher, right? ...
////       setupAndCopyFrom inside ENABLE_DEEP_COPY is going to use this when
////       it copies stuff over. Is that what I want?
//TacoTensor::TacoTensor(TacoTensor& other) {
//    this->operator=(other);
//}
//
//TacoTensor& TacoTensor::operator=(TacoTensor& other) {
//    if(this == &other) {
//        return *this;
//    }
//
//    // set order to 0 so copyFrom knows how big *this is
//    order = 0;
//
//    // set datatype, since copyFrom can't set that
//    // (taco_tensor_t only knows size of the datatype)
//    datatype = other.datatype;
//
//    // create a taco_tensor_t object from other
//    taco_tensor_t* temp = other.init_temp_c_ptr();
//    // copy the data to here
//    copyFrom(temp);
//    // free the taco_tensor_t object
//    other.deinit_temp_c_ptr(temp);
//
//    return *this;
//}

taco::TensorBase TacoTensor::copyToTaco() const {
    // this method is similar to copyFrom

    // First, create a TensorBase object that has the right format.
    std::vector<int> dimensionsOut;
    std::vector<int> modeOrderingOut;
    std::vector<taco::ModeFormatPack> modeFormatOut;
    std::vector<taco::ModeIndex> modeIndices;
    dimensionsOut.reserve(order);
    modeOrderingOut.reserve(order);
    modeFormatOut.reserve(order);
    modeIndices.reserve(order);

    size_t numVals = 1;
    for(int i = 0; i != order; ++i) {
        dimensionsOut.push_back(dimensions[i]);
        modeOrderingOut.push_back(modeOrdering[i]);
        if(modeTypes[i] == taco_mode_t::taco_mode_dense) {
            modeFormatOut.push_back(taco::Dense);

            int d = ((int*)(inds[i][0]->data))[0];
            modeIndices.emplace_back(std::vector<taco::Array>({ taco::makeArray({d}) }));

            numVals *= d;
        } else {
            modeFormatOut.push_back(taco::Sparse);

            int* pos = (int*)(inds[i][0]->data);
            int* idx = (int*)(inds[i][1]->data);

            auto size = pos[numVals];

            void* posOut = malloc(sizeof(int)*(numVals + 1));
            void* idxOut = malloc(sizeof(int)*size);
            std::memcpy(posOut, pos, sizeof(int)*(numVals + 1));
            std::memcpy(idxOut, idx, sizeof(int)*(size));

            modeIndices.emplace_back(std::vector<taco::Array>({
                    taco::Array(taco::type<int>(), posOut, numVals + 1, taco::Array::Free),
                    taco::Array(taco::type<int>(), idxOut, size,        taco::Array::Free)
            }));

            numVals = size;
        }
    }

    taco::Datatype datatypeOut((taco::Datatype::Kind(datatype)));
    taco::Format formatOut(modeFormatOut, modeOrderingOut);
    taco::Index indicesOut(formatOut, modeIndices);

    void* valsOut = malloc(csize * numVals);
    std::memcpy(valsOut, (void*)vals->data, csize*numVals);
    taco::Array valuesOut(datatypeOut, valsOut, numVals, taco::Array::Policy::Free);

    // Then get the storage object from it and set the indices and values
    taco::TensorBase ret(datatypeOut, dimensionsOut, formatOut);
    taco::TensorStorage& storage = ret.getStorage();
    storage.setIndex(indicesOut);
    storage.setValues(valuesOut);

    return ret;
}

void TacoTensor::callKernel(void* function, std::vector<taco_tensor_t*> ts) {
    // call function
    if(ts.size() == 1) {
        void(*f)(taco_tensor_t*);
        f = (void(*)(taco_tensor_t*)) function;
        f(ts[0]);
    } else if(ts.size() == 2) {
        void(*f)(taco_tensor_t*, taco_tensor_t*);
        f = (void(*)(taco_tensor_t*, taco_tensor_t*)) function;
        f(ts[0], ts[1]);
    } else if(ts.size() == 3) {
        void(*f)(taco_tensor_t*, taco_tensor_t*, taco_tensor_t*);
        f = (void(*)(taco_tensor_t*, taco_tensor_t*, taco_tensor_t*)) function;
        f(ts[0], ts[1], ts[2]);
    } else if(ts.size() == 4) {
        void(*f)(taco_tensor_t*, taco_tensor_t*, taco_tensor_t*, taco_tensor_t*);
        f = (void(*)(taco_tensor_t*, taco_tensor_t*, taco_tensor_t*, taco_tensor_t*)) function;
        f(ts[0], ts[1], ts[2], ts[3]);
    } else if(ts.size() == 5) {
        void(*f)(taco_tensor_t*, taco_tensor_t*, taco_tensor_t*, taco_tensor_t*, taco_tensor_t*);
        f = (void(*)(taco_tensor_t*, taco_tensor_t*, taco_tensor_t*, taco_tensor_t*, taco_tensor_t*)) function;
        f(ts[0], ts[1], ts[2], ts[3], ts[4]);
    } else {
        std::cout << "Uh-oh!\n";
        // TODO
    }
}

void TacoTensor::callKernel(void* function, std::vector<Handle<TacoTensor>> tensors) {
    std::vector<taco_tensor_t*> ts;
    ts.reserve(tensors.size());
    for(auto& handle: tensors) {
        ts.push_back(handle->init_temp_c_ptr());
    }

    callKernel(function, ts);

    // free the temporary memory used to construct the taco_tensor_t's
    // and reset the Handles in inds
    for(int i = 0; i != tensors.size(); ++i) {
        tensors[i]->deinit_temp_c_ptr(ts[i]);
    }
}


taco_tensor_t* TacoTensor::init_temp_c_ptr() {
    taco_tensor_t* ret = (taco_tensor_t*)malloc(sizeof(taco_tensor_t));

    ret->order = order;
    ret->csize = csize;
    ret->dimensions = dimensions.c_ptr();
    ret->mode_types = (taco_mode_t*)(modeTypes.c_ptr());
    ret->mode_ordering = modeOrdering.c_ptr();

    ret->indices = (uint8_t***)malloc(order * sizeof(uint8_t***));
    for(int j = 0; j != order; ++j) {
        if(ret->mode_types[j] == taco_mode_t::taco_mode_dense) {
            ret->indices[j] = (uint8_t**) malloc(1 * sizeof(uint8_t **));
            // inds[j][0] should point to an int array of size 1 containing
            // the size of this mode
            ret->indices[j][0] = (uint8_t*)(inds[j][0]->data);
        } else if(ret->mode_types[j] == taco_mode_t::taco_mode_sparse) {
            ret->indices[j] = (uint8_t**) malloc(2 * sizeof(uint8_t **));
            if(!inds[j][0].isNullPtr()) {
                ret->indices[j][0] = (uint8_t*)(inds[j][0]->data);
            }
            if(!inds[j][1].isNullPtr()) {
                ret->indices[j][1] = (uint8_t*)(inds[j][1]->data);
            }
        }
    }

    if(!vals.isNullPtr()) {
        ret->vals = (uint8_t*)(vals->data);
    }

    return ret;
}

void TacoTensor::deinit_temp_c_ptr(taco_tensor_t* t) {
    // this function should only be called if it is known that
    // inds[j][0], inds[j][1] and vals have a valid Ref Count Arr object
    for(int j = 0; j < t->order; j++) {
        if(t->mode_types[j] == taco_mode_t::taco_mode_dense) {
            // inds for dense modes should not be modified
        } else if(t->mode_types[j] == taco_mode_t::taco_mode_sparse) {
            // inds for sparse modes could be modified, so reset the handle
            inds[j][0] = (RefCountedObject<Arr>*)((char*)t->indices[j][0] - sizeof(Arr) - REF_COUNT_PREAMBLE_SIZE);
            inds[j][1] = (RefCountedObject<Arr>*)((char*)t->indices[j][1] - sizeof(Arr) - REF_COUNT_PREAMBLE_SIZE);
        }
        free(t->indices[j]);
    }

    // vals could have been modifed, so reset the handle
    vals = (RefCountedObject<Arr>*)((char*)t->vals - sizeof(Arr) - REF_COUNT_PREAMBLE_SIZE);

    free(t->indices);
    free(t);
}


// This function is similar to taco::UnpackTensorData
size_t TacoTensor::copyFrom(taco_tensor_t* other) {
    // if the new order is larger, allocate more space in for pointers
    // and vectors
    if(other->order > order) {
        // TODO: when resize works, use it
        dimensions = Vector<int32_t>(other->order, other->order); // .resize(other->order);
        modeOrdering = Vector<int32_t>(other->order, other->order); // .resize(other->order);
        modeTypes = Vector<mode_t>(other->order, other->order); // .resize(other->order);
        inds = Vector<Vector<Handle<Arr>>>(other->order, other->order); // .resize(other->order);
    }

    // copy over the metadata -- everything except the indices
    // and vals field
    order = other->order;
    csize = other->csize;
    std::memcpy(dimensions.c_ptr(), other->dimensions, sizeof(int32_t)*order);
    std::memcpy(modeOrdering.c_ptr(), other->mode_ordering, sizeof(int32_t)*order);
    std::memcpy(modeTypes.c_ptr(), other->mode_types, sizeof(mode_t)*order);

    // copy over vals    to Handle<Arr> vals and
    //           indices to Vector<Vector<Handle<Arr> > > inds
    size_t numVals = 1;
    for(int i = 0;  i != order; ++i) {
        taco_mode_t modeType = other->mode_types[i];
        if(modeType == taco_mode_t::taco_mode_dense) {
            inds[i] = Vector<Handle<Arr>>(1, 1); // .resize(1);
            inds[i][0] = makeObjectWithExtraStorage<Arr>(sizeof(int)*1);
            inds[i][0]->size = sizeof(int)*1;
            int* indsi0 = (int*)(inds[i][0]->data);
            indsi0[0] = ((int*)other->indices[i][0])[0];
            numVals *= indsi0[0];
        } else if(modeType == taco_mode_t::taco_mode_sparse) {
            inds[i] = Vector<Handle<Arr>>(2, 2); // .resize(2);

            int* otherPos = ((int*)other->indices[i][0]);
            int* otherIdx = ((int*)other->indices[i][1]);

            auto size = otherPos[numVals];

            // allocate memory in Arr's
            inds[i][0] = makeObjectWithExtraStorage<Arr>(sizeof(int)*(numVals+1)); // pos
            inds[i][1] = makeObjectWithExtraStorage<Arr>(sizeof(int)*size);        // idx

            inds[i][0]->size = sizeof(int)*(numVals+1);
            inds[i][1]->size = sizeof(int)*size;

            int* pos = (int*)(inds[i][0]->data);
            int* idx = (int*)(inds[i][1]->data);
            // and copy it over
            std::memcpy(pos, otherPos, sizeof(int)*(numVals + 1));
            std::memcpy(idx, otherIdx, sizeof(int)*size);

            numVals = size;
        } else {
            // TODO throw error
        }
    }

    vals = makeObjectWithExtraStorage<Arr>(csize * numVals);
    vals->size = csize * numVals;
    std::memcpy((void*)vals->data, (void*)other->vals, csize*numVals);
    return numVals;
}


