#pragma once

#include <map>
#include <vector>
#include <string>
#include <utility>

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

            // TODO what if compile fails?
            void* function = theTacoModule->compileAssignment(assignment);
            theTacoModule->functionMap[str] = function;
            return function;
        } else {
            return theTacoModule->functionMap[str];
        }
    }

private:
    // Only call these private functions with this == theTacoModule
    void* compileAssignment(taco::Assignment& assignment) {
        if(target == nullptr) {
            target = std::make_shared<taco::Target>(taco::getTargetFromEnvironment());
        }

        std::string name = "CompiledByTacoModuleFunction";
        void* libHandle = compile(assignment, name);
        libHandles.push_back(libHandle);
        void* function = dlsym(libHandle, name.data());
        if(!function) {
            std::cout << "uh-oh no function" << std::endl;
            return nullptr;
        }
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
        stmt = parallelizeOuterLoop(stmt);

        taco::ir::Stmt compute = lower(stmt, name, true, true);

        std::string tmpdir = taco::util::getTmpdir();
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
            std::cout << "Uh-oh 1\n";
            //TODO
        }

        std::string cmdReplace = "objcopy --redefine-sym malloc=tacoMalloc --redefine-sym realloc=tacoRealloc " +
            fullpathObj + " " + fullpathObjReplaced;
        system(cmdReplace.data());
        std::string fullpath = prefix + ".so";
        // TODO: use a macro set by CMake to specify where this file is
        // TODO: make the TacoMemory stuff used to produce the .o file below live in this directory.
        //       That will also mean that TestTaco's Arr will need to be moved to live in pdb's builtIn
        //       objects
        std::string mallocs = "./CMakeFiles/TacoMemory.dir/applications/TestTaco/sharedLibraries/source/TacoMemory.cc.o";
        std::string cmdLink = "cc -shared -fPIC -o "+fullpath+" "+fullpathObjReplaced+" "+mallocs;
        err = system(cmdLink.data());
        if(err != 0) {
            std::cout << "Uh-oh 2\n";
            //TODO
        }

        void* lib_handle = dlopen(fullpath.data(), RTLD_NOW | RTLD_LOCAL);
        if(!lib_handle) {
            std::cout << "Uh-oh 3\n";
            //TODO
        }

        return lib_handle;
    }
};

} // namespace pdb
