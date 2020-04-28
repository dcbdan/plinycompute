#pragma once

#include <map>
#include <vector>
#include <string>
#include <utility>

#include "PDBLogger.h"

#include "taco/codegen/module.h"
#include "taco/target.h"
#include "taco/ir/ir.h"
#include "taco/util/env.h"

// for assignment -> stmt
#include <taco/index_notation/transformations.h>
// for lower: stmt -> stmt to then put in Module
#include "taco/lower/lower.h"

#include "TacoMod.h"

namespace pdb {

class TacoModuleMap;

extern bool inSharedLibrary;
extern TacoModuleMap* theTacoModule;

class TacoModuleMap {
    std::shared_ptr<taco::Target> target;

    PDBLoggerPtr logger;

    std::shared_ptr<std::string> tmpdirPtr;

    // One library is compiled per function pointer
    std::vector<void*> libHandles;

    std::map<std::string, void*> functionMap;

    pthread_mutex_t myLock;
public:
    /// Create a module for some target
    TacoModuleMap() : target(nullptr) {
    }

    ~TacoModuleMap() {
        const LockGuard guard{theTacoModule->myLock};

        if (!inSharedLibrary) {
            for(void* libHandle: theTacoModule->libHandles) {
                if(libHandle) {
                    int res = dlclose(libHandle);
                    if(res != 0) {
                        std::cout << dlerror() << "\n";
                    }
                }
            }
        }

        theTacoModule->libHandles.clear();
    }

    void setTarget(taco::Target otherTarget) {
        const LockGuard guard{theTacoModule->myLock};

        if(target == nullptr) {
            target = std::make_shared<taco::Target>(otherTarget);
        } else {
            *target = otherTarget;
        }
    }

    // Get a function pointer that can compute an Assignment object. If
    // there is no such function pointer, then compile it.
    // TODO: Should a typed function object be returned instead?
    // TODO: is by reference necessary?
    void* operator[](taco::Assignment& assignment) {
        std::stringstream buffer;
        AssignmentNotationPrinter printer(buffer);
        printer.print(assignment);
        std::string const& str = buffer.str();

        // I'm assuming that a string maps to an assignment perfectly,
        // that is, two assignment statements with the same string
        // representation should map to the same computation. (assuming
        // that AssignmentNotationPrinter from TacoMod.h is being used
        // to construct the string, not taco::IndexNotationPrinter, which
        // is what is used in operator<< for the taco::Assignment base class)
        // However, if this proves not to be the case, TacoMod.h also
        // contains the infrastructure to compare different Assignments
        // as equal. It is commented out.
        if(theTacoModule->functionMap.count(str) == 0) {
            // The function needs to be compiled, so get a lock
            const LockGuard guard{theTacoModule->myLock};

            // Check that the function hasn't been compiled in the
            // meantime
            if(theTacoModule->functionMap.count(str) != 0) {
                return theTacoModule->functionMap[str];
            }

            // set the target and logger here
            theTacoModule->setDefaultsIfNeccessary();

            // TODO what if compile fails?
            void* function = theTacoModule->compileAssignmentAndSetGlobalVariables(assignment);
            theTacoModule->functionMap[str] = function;
            return function;
        } else {
            return theTacoModule->functionMap[str];
        }
    }

private:
    // Only call these private functions with this == theTacoModule
    void setDefaultsIfNeccessary() {
        if(target == nullptr) {
            target = std::make_shared<taco::Target>(taco::getTargetFromEnvironment());
        }
        if(logger == nullptr) {
            logger = std::make_shared<PDBLogger>("tacomodulemap.log");
        }
        if(tmpdirPtr == nullptr) {
            tmpdirPtr = std::make_shared<std::string>("taco_tmp/");
        }
    }

    void* compileAssignmentAndSetGlobalVariables(taco::Assignment& assignment) {
        std::string name = "CompiledByTacoModuleFunction";
        void* libHandle = compile(assignment, name);
        if(!libHandle) {
            return nullptr;
        }

        libHandles.push_back(libHandle);
        void* function = dlsym(libHandle, name.data());
        const char* dlsymError = dlerror();
        if(dlsymError) {
            std::string error(dlsymError);
            logger->error("Could not open function "+name+": "+error);
            return nullptr;
        }

        // set the global variables (TODO: why did I not need to do this when
        // I was just running with the object model?)
        std::string getInstance = "setAllGlobalVariables";
        typedef void setGlobalVars(Allocator*, VTableMap*, void*, void*, TacoModuleMap*);
        setGlobalVars* setGlobalVarsFunc = (setGlobalVars*)dlsym(
            libHandles.back(),
            getInstance.c_str());
        const char* dlsymErrorSetGlobal = dlerror();
        if(dlsymErrorSetGlobal) {
            std::string error(dlsymErrorSetGlobal);
            logger->error("Could not open function "+name+": "+error);
            return nullptr;
        }

        setGlobalVarsFunc(mainAllocatorPtr, theVTable, stackBase, stackEnd, theTacoModule);

        return function;
    }

    void compileToSource(taco::ir::Stmt& compute, std::string& tmpdir, std::string& libname) {
        taco::ir::Module m(*target);
        m.addFunction(compute);
        m.compileToSource(tmpdir, libname);
    }

    void* compile(taco::Assignment& assignment, std::string name) {
        taco::IndexStmt stmt = makeConcreteNotation(makeReductionNotation(assignment));
        stmt = reorderLoopsTopologically(stmt);
        stmt = insertTemporaries(stmt);
        // don't do this? The assumption is that the taco build was without
        // openmp and cuda
        //stmt = parallelizeOuterLoop(stmt);

        taco::ir::Stmt compute = lower(stmt, name, true, true);

        std::string tmpdir = *tmpdirPtr.get();
        std::cout << tmpdir << "**********************************" << std::endl << std::endl;
        std::string libname = "TacoModuleLib"+std::to_string(libHandles.size());

        std::string prefix = tmpdir+libname;
        std::string fullpathObj = prefix + ".o";
        std::string fullpathObjReplaced = prefix + "_replaced.o";

        std::string cc;
        std::string cflags;
        std::string file_ending;

        cc = taco::util::getFromEnv(target->compiler_env, target->compiler);
        cflags = taco::util::getFromEnv("TACO_CFLAGS",
          "-O3 -ffast-math -std=c99") + " -c -fPIC " +
          "-fno-builtin-malloc -fno-builtin-realloc";
        // no-builtin options makes it so that there are symbols for
        // objcopy to replace

        file_ending = ".c";

        std::string cmd = cc + " " + cflags + " " +
          prefix + file_ending + " " + " " +
          "-o " + fullpathObj + " -lm";

        // open the output file & write out the source
        compileToSource(compute, tmpdir, libname);

        // now compile it
        int err = system(cmd.data());
        if(err != 0) {
            logSystemCallError(cmd, err);
            return nullptr;
        }

        std::string cmdReplace = "objcopy --redefine-sym malloc=tacoMalloc --redefine-sym realloc=tacoRealloc " +
            fullpathObj + " " + fullpathObjReplaced;
        err = system(cmdReplace.data());
        if(err != 0) {
            logSystemCallError(cmdReplace, err);
            return nullptr;
        }

        std::string fullpath = prefix + ".so";
        // TODO: use a macro set by CMake to specify where this file is
        // TODO: make the TacoMemory stuff used to produce the .o file below live in this directory.
        //       That will also mean that TestTaco's Arr will need to be moved to live in pdb's builtIn
        //       objects
        std::string mallocs = "./CMakeFiles/TacoMemory.dir/applications/TestTaco/sharedLibraries/source/TacoMemory.cc.o";
        std::string cmdLink = "cc -shared -fPIC -o "+fullpath+" "+fullpathObjReplaced+" "+mallocs;
        err = system(cmdLink.data());
        if(err != 0) {
            logSystemCallError(cmdLink, err);
            return nullptr;
        }

        void* lib_handle = dlopen(fullpath.data(), RTLD_NOW | RTLD_LOCAL);
        if(!lib_handle) {
            std::string dlsym_error(dlerror());
            logger->error("Could not open "+fullpath+": "+dlsym_error);
            return nullptr;
        }

        return lib_handle;
    }

    void logSystemCallError(std::string const& command, int error) {
        logger->error(
            "System command, '"+command+" failed with status "+
            std::to_string(error));
    }
};

} // namespace pdb
