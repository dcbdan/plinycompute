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


namespace pdb {

// How about a TacoModule that is a dictionary of taco::ir::Module?
// That seems modular enough. But the problem is that compiling it isn't
// the same..


//  Dimension M, N, O;
//
//  IndexVar i("i"), j("j"), k("k");
//
//  TensorVar A("A", Type(Float32,{M,N}),   CSR);
//  TensorVar B("B", Type(Float32,{M,N,O}), {Dense, Dense, Sparse});
//  TensorVar c("c", Type(Float32,{O}),     Dense);

class TacoModule {
    taco::Target target;
    void* lib_handle;
public:
    /// Create a module for some target
    TacoModule(taco::Target target=taco::getTargetFromEnvironment())
      : lib_handle(nullptr), target(target) {
    }

    ~TacoModule() {
        if(lib_handle) {
            dlclose(lib_handle);
        }
    }

    // compile an assigment
    void* compile(taco::Assignment& assignment) {
        std::string name = "asd";
        compile(assignment, name); // name TODO
        void* function = dlsym(lib_handle, name.data());
        if(!function) {
            std::cout << "uh-oh no function" << std::endl;
            return nullptr;
        }
        return function;
    }

private:

    void compileToSource(taco::ir::Stmt& compute, std::string& tmpdir, std::string& libname) {
        taco::ir::Module m;
        m.addFunction(compute);
        m.compileToSource(tmpdir, libname);
    }

    void compile(taco::Assignment& assignment, std::string name) {
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

        // use dlsym() to open the compiled library
        if (lib_handle) {
          dlclose(lib_handle);
        }

        lib_handle = dlopen(fullpath.data(), RTLD_NOW | RTLD_LOCAL);
        if(!lib_handle) {
            std::cout << "Uh-oh 3\n";
            //TODO
        }
    }
};


}
