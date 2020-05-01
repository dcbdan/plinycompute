#include <random>

#include <boost/program_options.hpp>
#include <PDBClient.h>
#include <GenericWork.h>

#include "sharedLibraries/headers/TacoTensor.h"
#include "sharedLibraries/headers/TacoTensorBlock.h"
#include "sharedLibraries/headers/TacoTensorBlockMeta.h"

#include "sharedLibraries/headers/TacoMatMulAggregation.h"
#include "sharedLibraries/headers/TacoMatMulJoin.h"

#include "sharedLibraries/headers/TacoScanner.h"
#include "sharedLibraries/headers/TacoWriter.h"

#include <taco/format.h>
#include <taco/type.h>
#include <taco/tensor.h>

using namespace pdb;
namespace po = boost::program_options;

void countSet(pdb::PDBClient& pdbClient, std::string db, std::string set) {
    int count = 0;
    auto it = pdbClient.getSetIterator<TacoTensorBlock>(db, set);
    while(it->hasNextRecord()) {
        it->getNextRecord();
        count++;
    }
    std::cout << "COUNT:     " << count << std::endl;
}

void printSet(pdb::PDBClient& pdbClient, std::string db, std::string set) {
    int count = 0;
    auto it = pdbClient.getSetIterator<TacoTensorBlock>(db, set);
    while(it->hasNextRecord()) {
        Handle<TacoTensorBlock> block = it->getNextRecord();

        TacoTensorBlockMeta& blockMeta = block->getKeyRef();
        TacoTensor& value = block->getValueRef();

        std::cout << "BLOCK:  " <<
            blockMeta.getIndex()[0] << ", " <<
            blockMeta.getIndex()[1] << std::endl;

        std::cout << "VALUE:  \n";
        std::cout << value.copyToTaco() << std::endl;
        std::cout << "DIAGNOSTICS:  \n";
        value.printDiagnostics();

        count++;
    }
    std::cout << "COUNT:     " << count << std::endl;
}

// Create a blocked
//   (blockNumRows x numRowsInBlock, blockNumCols x numColsInBlock)
// matrix
void initMatrix(
    pdb::PDBClient &pdbClient,
    const std::string &set,
    const size_t blockSize,
    const float &sparsity,
    taco::Format format,
    const uint32_t &numRowsInBlock,
    const uint32_t &numColsInBlock,
    const uint32_t &blockNumRows,
    const uint32_t &blockNumCols)
{
    std::cout << "init enter " << set << std::endl;

    // create a closure that fills out a taco tensor
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution isNonzero(1 - sparsity);
    auto fill = [&](taco::TensorBase& tensor) {
        for(int i = 0; i != numRowsInBlock; ++i) {
            for(int j = 0; j != numColsInBlock; ++j) {
                if(isNonzero(gen)) {
                    tensor.insert({i,j}, 1.0); //1.0*(i*numRowsInBlock+j));
                }
            }
        }

        tensor.pack();
    };

    uint32_t blockId = 0;
    uint32_t numBlocks = blockNumRows * blockNumCols;
    while(blockId != numBlocks) {
        const pdb::UseTemporaryAllocationBlock tempBlock{blockSize * 1024 * 1024};

        using VecBlock = Vector<Handle<TacoTensorBlock>>;

        // but each block here
        Handle<VecBlock> data = pdb::makeObject<VecBlock>();

        try {
            for(; blockId != numBlocks; ++blockId) {
                // create and fill out a taco tensor
                taco::TensorBase tensorTaco(
                    taco::type<double>(),
                    {(int)numRowsInBlock, (int)numColsInBlock},
                    format);
                fill(tensorTaco);

                // copy it to pdb
                Handle<TacoTensor> tensor = makeObject<TacoTensor>(tensorTaco);

                // build the block meta data
                uint32_t r = blockId % blockNumRows;
                uint32_t c = blockId / blockNumRows;
                using Meta = TacoTensorBlockMeta;
                Handle<Meta> meta = makeObject<Meta>(std::vector<uint32_t>({r, c}));

                // now store the full TacoTensorBlock handle
                data->push_back(makeObject<TacoTensorBlock>(tensor, meta));
            }
        } catch( pdb::NotEnoughSpace &n) {
        }

        // init the records
        getRecord(data);

        // send the data
        std::cout << data->size() << std::endl;
        pdbClient.sendData<TacoTensorBlock>("myData", set, data);
    }
}

int main(int argc, char* argv[]) {
    po::options_description desc{"Options"};

    size_t blockSize;
    uint32_t Bi, Bj, Bk, bi, bj, bk;
    double sparsity1, sparsity2;

    // specify the options
    desc.add_options()("help,h", "Help screen");
    desc.add_options()("blockSize", po::value<size_t>(&blockSize)->default_value(1024),
        "Block size for allocation");
    desc.add_options()("Bi", po::value<uint32_t>(&Bi)->default_value(200),"");
    desc.add_options()("Bj", po::value<uint32_t>(&Bj)->default_value(200),"");
    desc.add_options()("Bk", po::value<uint32_t>(&Bk)->default_value(1),"");
    desc.add_options()("bi", po::value<uint32_t>(&bi)->default_value(10000),"");
    desc.add_options()("bj", po::value<uint32_t>(&bj)->default_value(10000),"");
    desc.add_options()("bk", po::value<uint32_t>(&bk)->default_value(1),"");
    desc.add_options()("sparsity1", po::value<double>(&sparsity1)->default_value(0.9),"");
    desc.add_options()("sparsity2", po::value<double>(&sparsity2)->default_value(0.0),"");

    // grab the options
    po::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    // did somebody ask for help?
    if (vm.count("help")) {
        std::cout << desc << '\n';
        return 0;
    }

    // make a client
    pdb::PDBClient pdbClient(8108, "localhost");

    // now, register a type for user data
    pdbClient.registerType("libraries/libArr.so");
    pdbClient.registerType("libraries/libTacoTensor.so");
    pdbClient.registerType("libraries/libTacoTensorBlock.so");
    pdbClient.registerType("libraries/libTacoTensorBlockMeta.so");
    pdbClient.registerType("libraries/libTacoMatMulAggregation.so");
    pdbClient.registerType("libraries/libTacoMatMulJoin.so");
    pdbClient.registerType("libraries/libTacoScanner.so");
    pdbClient.registerType("libraries/libTacoWriter.so");

    const pdb::UseTemporaryAllocationBlock tempBlock{blockSize * 1024 * 1024};

    // now, create a new database
    pdbClient.createDatabase("myData");

    // now, create the input and output sets
    pdbClient.createSet<TacoTensorBlock>("myData", "A");
    pdbClient.createSet<TacoTensorBlock>("myData", "B");
    pdbClient.createSet<TacoTensorBlock>("myData", "C");

    initMatrix(pdbClient, "A", blockSize, sparsity1, {taco::dense, taco::sparse},  bi, bj, Bi, Bj);
    initMatrix(pdbClient, "B", blockSize, sparsity2, {taco::dense, taco::dense},  bj, bk, Bj, Bk);

    std::cout << "FINISHED INIT\n";

    countSet(pdbClient, "myData", "A");
    countSet(pdbClient, "myData", "B");

    //printSet(pdbClient, "myData", "A");
    //printSet(pdbClient, "myData", "B");

    ////////////////////////////////////////////
    //auto itA = pdbClient.getSetIterator<TacoTensorBlock>("myData", "A");
    //Handle<TacoTensorBlock> blockA = itA->getNextRecord();
    //Handle<TacoTensor> A = blockA->getValue();

    //auto itB = pdbClient.getSetIterator<TacoTensorBlock>("myData", "B");
    //Handle<TacoTensorBlock> blockB = itB->getNextRecord();
    //Handle<TacoTensor> B = blockB->getValue();

    //Handle<TacoTensor> C = makeObject<TacoTensor>(
    //    A->getDatatype(),
    //    std::vector<int>({(int)bi,1}),
    //    taco::Format({taco::dense, taco::dense}));

    //std::string name = "CompiledByTacoModuleFunction";
    //std::string getInstance = "setAllGlobalVariables";
    //std::string filename = "taco_tmp/TacoModuleLib0.so";

    //void* libHandle = dlopen(filename.data(), RTLD_NOW | RTLD_LOCAL);

    //typedef void setGlobalVars(Allocator*, VTableMap*, void*, void*, TacoModuleMap*);
    //setGlobalVars* setGlobalVarsFunc = (setGlobalVars*)dlsym(
    //    libHandle,
    //    getInstance.c_str());
    //setGlobalVarsFunc(mainAllocatorPtr, theVTable, stackBase, stackEnd, theTacoModule);

    //void* function = dlsym(libHandle, name.data());

    //TacoTensor::callKernel(function, {C, A, B});

    //std::cout << "The answer: " << std::endl;
    //std::cout << (C->copyToTaco()) << std::endl;
    ////////////////////////////////////////////

    Handle <Computation> readA = makeObject <TacoScanner>("myData", "A");
    Handle <Computation> readB = makeObject <TacoScanner>("myData", "B");
    Handle <Computation> join = makeObject <TacoMatMulJoin>();
    join->setInput(0, readA);
    join->setInput(1, readB);
    Handle<Computation> aggregation = makeObject<TacoMatMulAggregation>();
    aggregation->setInput(join);
    Handle<Computation> writer = makeObject<TacoWriter>("myData", "C");
    writer->setInput(aggregation);

    pdbClient.executeComputations({ writer });

    // get the set and print
    printSet(pdbClient, "myData", "C");

    // shutdown the server
    pdbClient.shutDownServer();

    return 0;
}
