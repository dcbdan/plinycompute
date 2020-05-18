#pragma once

#include <Object.h>
#include <Nothing.h>
#include <Allocator.h>
#include <VTableMap.h>
#include <TacoModuleMap.h>

#include "Arr.h"

namespace pdb {
    extern VTableMap* theVTable;
    extern void* stackBase;
    extern void* stackEnd;
    extern bool inSharedLibrary;
    extern TacoModuleMap* theTacoModule;
}

extern "C" {
    void* tacoMalloc(size_t size);
    void* tacoRealloc(void* ptr, size_t new_size);
    void* tacoMallocCount(size_t size);
    void* tacoReallocCount(void* ptr, size_t new_size);
    void setAllGlobalVariables(
        pdb::Allocator* newAllocator,
        pdb::VTableMap* theVTableIn,
        void* stackBaseIn,
        void* stackEndIn,
        pdb::TacoModuleMap* theTacoModuleIn);
} // extern "C"

