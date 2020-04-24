#pragma once

#include <map>
#include <vector>
#include <string>
#include <utility>


#include "taco/target.h"
#include "taco/ir/ir.h"
#include "taco/util/env.h"

// for assignment -> stmt
#include <taco/index_notation/transformations.h>
// for lower: stmt -> stmt to then put in Module
#include "taco/lower/lower.h"

#include "TacoMod.h"

namespace pdb {

// TODO: Should TacoModule constrain to only formats with dynamic dimension size?
class TacoModule {
    taco::Target target;

    // One library is compiled per function pointer
    vector<void*> lib_handles;

    std::map<std::string, void*> functionPointers;
public:
    /// Create a module for some target
    TacoModule(taco::Target target=taco::getTargetFromEnvironment())
      : target(target) {
    }

    ~TacoModule() {
        for(void* lib_handle: lib_handles) {
            if(lib_handle) {
                dlclose(lib_handle);
            }
        }
    }

    // Get a function pointer that can compute an Assignment object. If
    // such there is no such function pointer, then compile it.
    // TODO: Should a typed function object be returned instead?
    // TODO: is by reference necessary?
    void* operator[](taco::Assignment& assignment) {
        std::stringstream buffer;
        tacomod::AssignmentNotationPrinter printer(buffer);
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
        if(functionPointers.count(str) == 0) {
            // TODO what if compile fails?
            void* function = compileAssignment(assignment);
            functionPointers[str] = function;
            return function;
        } else {
            return functionPointers[str];
        }
    }

private:
    // compile an assigment
    void* compileAssignment(taco::Assignment& assignment) {
        std::string name = "CompiledByTacoModuleFunction";
        void* lib_handle = compile(assignment, name); // name TODO
        lib_handles.push_back(lib_handle);
        void* function = dlsym(lib_handle, name.data());
        if(!function) {
            std::cout << "uh-oh no function" << std::endl;
            return nullptr;
        }
        return function;
    }

    void compileToSource(taco::ir::Stmt& compute, std::string& tmpdir, std::string& libname) {
        taco::ir::Module m;
        m.addFunction(compute);
        m.compileToSource(tmpdir, libname);
    }

    void* compile(taco::Assignment& assignment, std::string name) {
        taco::IndexStmt stmt = makeConcreteNotation(makeReductionNotation(assignment));
        stmt = reorderLoopsTopologically(stmt);
        stmt = insertTemporaries(stmt);
        stmt = parallelizeOuterLoop(stmt);

        taco::ir::Stmt compute = lower(stmt, name, true, true);

        std::string tmpdir = "./";   // TODO
        std::string libname = "aSD"; // TODO

        string prefix = tmpdir+libname;
        string fullpathObj = prefix + ".o";
        string fullpathObjReplaced = prefix + "_replaced.o";

        string cc;
        string cflags;
        string file_ending;

        cc = taco::util::getFromEnv(target.compiler_env, target.compiler);
        cflags = taco::util::getFromEnv("TACO_CFLAGS",
          "-O3 -ffast-math -std=c99") + " -c -fPIC " +
          "-fno-builtin-malloc -fno-builtin-realloc";
        // no-builtin options makes it so that there are symbols for
        // objcopy to replace

        file_ending = ".c";

        string cmd = cc + " " + cflags + " " +
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
        string fullpath = prefix + ".so";
        string mallocs = "./CMakeFiles/TacoMemory.dir/applications/TestTaco/sharedLibraries/source/TacoMemory.cc.o";
        string cmdLink = "cc -shared -fPIC -o "+fullpath+" "+fullpathObjReplaced+" "+mallocs;
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


}
