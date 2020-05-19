#include "../headers/TacoMemory.h"

#include "Handle.h"

using namespace pdb;

// TODO: GET_V_TABLE sets global variable in the shared library...
//       I'm just using the object file from this file..
//       how do the global variables "get set"?

void* tacoMalloc(size_t size) {
    RefCountedObject<Arr>* ref = makeObjectWithExtraStorage<Arr>(size);
    // At this point in time, the reference count of ref is zero.
    // That is ok. When it gets converted to a handle, the handle
    // will increment the reference account. The assumed invariant being
    // that ref lives on the allocation block and the handle it gets
    // set to lives on that same allocation block.

    // We want the following to work (again, same allocation block, so
    // think in a lambda opreation) --
    //   void* ptr = tacoMalloc(size);
    //   ptr = tacoRealloc(ptr, new_size);
    // This could happen in a Taco kernel.

    // Record size of allocated memory in Arr
    Arr* arr = (Arr*)(CHAR_PTR(ref) + REF_COUNT_PREAMBLE_SIZE);
    arr->size = size;

    // return the data
    return (void*)(arr->data);
}

// ptr cannot be null similarly to realloc.
void* tacoRealloc(void* ptr, size_t new_size) {
    // Get RefCountedObject that ptr is from
    RefCountedObject<Arr>* refIn = (RefCountedObject<Arr>*)
        (CHAR_PTR(ptr) - sizeof(Arr) - REF_COUNT_PREAMBLE_SIZE);
    // Get Arr* object
    Arr* arrIn = (Arr*)(CHAR_PTR(refIn) + REF_COUNT_PREAMBLE_SIZE);
    // Invariant: ptr comes from a RefCountedObject<Arr>* shifted to
    //            point to the raw data

    if(arrIn->size >= new_size) {
        // not going to set arrIn->size to new_size on purpose
        // as the memory is gratis, take it
        return ptr;
    }

    // No check will be made to verify that the memory does not overlap.
    // So we can assume that the only case is that a copy has to made.

    // First allocate some memory.
    void* ptrOut = tacoMalloc(new_size);

    // copy it over; note that ptr == arr->data
    std::memmove(ptrOut, ptr, arrIn->size);

    // We need the type info for when the counter gets decremented
    PDBTemplateBase typeInfo;
    typeInfo.setup<Arr>();

    // We assume that refIn has a reference count of zero. This is because
    // all calls to this remote malloc and realloc should be called
    // before the ref gets converted into a handle. Once the final ref gets
    // converted into a handle, the handle will increment the reference count.
    // Note that for this to happen, it is required for both the handle and the
    // ref to be on the same allocation block.

    // Since refIn has a count of zero, decrementing the unsigned value
    // won't further delete it. To delete it, we increment the reference
    refIn->incRefCount();
    // Now, we decrement it, and in doing so, the RefCountedObject handles
    // deleting itself
    refIn->decRefCount(typeInfo);

    return ptrOut;
}

void setAllGlobalVariables(
    pdb::Allocator* newAllocator,
    pdb::VTableMap* theVTableIn,
    void* stackBaseIn,
    void* stackEndIn,
    pdb::TacoModuleMap* theTacoModuleIn)
{
    pdb::stackBase        = stackBaseIn;
    pdb::mainAllocatorPtr = newAllocator;
    pdb::stackEnd         = stackEndIn;
    pdb::theVTable        = theVTableIn;
    pdb::inSharedLibrary  = true;
    pdb::theTacoModule    = theTacoModuleIn;
}

