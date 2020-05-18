#pragma once

#include <map>
#include <vector>
#include <string>
#include <utility>
#include <fstream>

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

    // A map from assignment string represenation to
    // (compute function, assemble function). Here
    // the assemble function is doesn't allocate the memory
    // like in taco, but it counts the required memory that
    // will be necessary to a call to the compute function
    std::map<std::string, std::pair<void*, void*>> functionMap;

    pthread_mutex_t myLock;
public:
    /// Create a module for some target
    TacoModuleMap() {
    }

    ~TacoModuleMap() {
        if(this == theTacoModule) {

            const LockGuard guard{myLock};

            if (!inSharedLibrary) {
                for(void* libHandle: libHandles) {
                    if(libHandle) {
                        int res = dlclose(libHandle);
                        if(res != 0) {
                            std::cout << dlerror() << "\n"; // TODO
                        }
                    }
                }
            }

            theTacoModule->libHandles.clear();
        }
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
    std::pair<void*, void*> operator[](taco::Assignment& assignment) {
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
            theTacoModule->functionMap[str] = theTacoModule->compileAndSetup(assignment);

            return theTacoModule->functionMap[str];
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

    void* getFunction(void* libHandle, std::string const& name) {
        void* out = dlsym(libHandle, name.data());
        const char* dlsymError = dlerror();
        if(dlsymError) {
            std::string error(dlsymError);
            logger->error("Could not open function "+name+": "+error);
            return nullptr;
        }
        return out;
    }

    void* compile(
        taco::Assignment& assignment,
        std::string const& computeName,
        std::string const& assembleName)
    {
        using namespace std;

        taco::IndexStmt stmt = makeConcreteNotation(makeReductionNotation(assignment));
        stmt = reorderLoopsTopologically(stmt);
        stmt = insertTemporaries(stmt);
        // don't do this? The assumption is that the taco build was without
        // openmp and cuda
        //stmt = parallelizeOuterLoop(stmt);

        // lower(stmt, functionName, assemble, compute)
        taco::ir::Stmt compute  = lower(stmt, computeName,  true, true);
        taco::ir::Stmt assemble = lower(stmt, assembleName, true, false);

        string tmpdir = *tmpdirPtr.get();
        string libname = "TacoModuleLib"+std::to_string(libHandles.size());
        string prefix = tmpdir+libname;
        string fullpath = prefix + ".so";
        std::string sourceOrig = prefix + "_taco" + ".c";
        std::string source = prefix + ".c";

        taco::ir::Module m(*target);
        m.addFunction(compute);
        m.addFunction(assemble);

        // print out the file to tmpdir/libname.c
        m.compileToSource(tmpdir, libname + "_taco");

        ifstream fileIn(sourceOrig);
        ofstream fileOut(source);

        // add the declaration of the alloc functions
        fileOut <<
            "#include \"stddef.h\"\n"                               <<
            "void* tacoMalloc(size_t size);\n"                      <<
            "void* tacoRealloc(void* ptr, size_t new_size);\n"      <<
            "void* tacoMallocCount(size_t size);\n"                 <<
            "void* tacoReallocCount(void* ptr, size_t new_size);\n";

        // replace all malloc/realloc strings in file with
        // tacoMalloc/tacoRealloc in the compute function, or
        // tacoMallocCount/tacoReallocCount in the assemble function

        auto replace = [](
            std::string& line,
            std::string const& from,
            std::string const& to)
        {
            size_t pos = line.find(from);
            if(pos == std::string::npos) {
                return;
            }

            line = line.replace(pos, from.size(), to);
        };

        std::string line;
        bool inCompute = false;
        bool inAssemble = false;
        while(std::getline(fileIn, line)) {
            if(inCompute) {
                replace(line, "malloc",  "tacoMalloc");
                replace(line, "realloc", "tacoRealloc");
            }
            if(inAssemble) {
                replace(line, "malloc",  "tacoMallocCount");
                replace(line, "realloc", "tacoReallocCount");
            }
            if(line.find(computeName) != std::string::npos) {
                inCompute = true;
                inAssemble = false;
            }
            if(line.find(assembleName) != std::string::npos) {
                inAssemble = true;
                inCompute = false;
            }

            fileOut << line << "\n";
        }
        fileIn.close();
        fileOut.close();

        std::string cc;
        std::string cflags;
        std::string file_ending;
        // TODO: use a macro and set it with cmake
        std::string tacoMemoryObj = "./CMakeFiles/TacoMemory.dir/applications/TestTaco/sharedLibraries/source/TacoMemory.cc.o";

        cc = taco::util::getFromEnv(target->compiler_env, target->compiler);
        cflags = taco::util::getFromEnv("TACO_CFLAGS",
          "-O3 -ffast-math -std=c99") + " -shared -fPIC ";

        std::string cmd = cc + " " + cflags + " " +
          source + " " + tacoMemoryObj + " " +
          "-o " + fullpath + " -lm ";

        // now compile it
        int err = system(cmd.data());
        if(err != 0) {
            logSystemCallError(cmd, err);
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

    std::pair<void*, void*> compileAndSetup(taco::Assignment& assignment) {
        std::string compute = "CompiledByTacoModuleMapCompute";
        std::string assemble = "CompiledByTacoModuleMapAssemble";
        void* libHandle = compile(assignment, compute, assemble);

        libHandles.push_back(libHandle);

        // try getting compute and assmeble
        void* computeF = getFunction(libHandle, compute);
        if(computeF == nullptr) {
            return std::make_pair(nullptr, nullptr);
        }
        void* assembleF = getFunction(libHandle, assemble);
        if(computeF == nullptr) {
            return std::make_pair(nullptr, nullptr);
        }

        std::string setGlobals = "setAllGlobalVariables";
        typedef void setGlobalVars(Allocator*, VTableMap*, void*, void*, TacoModuleMap*);
        void* globalF = getFunction(libHandle, setGlobals);
        if(globalF == nullptr) {
            return std::make_pair(nullptr, nullptr);
        }
        setGlobalVars* setGlobalVarsFunc = (setGlobalVars*)globalF;

        // set the global variables
        setGlobalVarsFunc(mainAllocatorPtr, theVTable, stackBase, stackEnd, theTacoModule);

        return std::make_pair(computeF, assembleF);
    }

    void logSystemCallError(std::string const& command, int error) {
        logger->error(
            "System command, '"+command+" failed with status "+
            std::to_string(error));
    }
};

} // namespace pdb
