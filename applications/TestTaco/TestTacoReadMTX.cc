#include <PDBClient.h>

#include "sharedLibraries/headers/TacoTensor.h"
#include "sharedLibraries/headers/TacoTensorBlock.h"
#include "sharedLibraries/headers/TacoTensorBlockMeta.h"

#include "sharedLibraries/headers/TacoReadMTX.h"

#include "tensorAlgebraDSL/headers/NLexer.h"
#include "tensorAlgebraDSL/headers/NParser.h"
#include "tensorAlgebraDSL/headers/NParserTypes.h"

#include <string>

#include "taco/storage/file_io_mtx.h"
#include "taco.h"

using std::string;

NProgramPtr myParse(string& parseMe) {
    // now parse it
    yyscan_t scanner;
    TTLexerExtra extra { "" };
    ttlex_init_extra (&extra, &scanner);
    const YY_BUFFER_STATE buffer { tt_scan_string (parseMe.data(), scanner) };
    NProgram *final = nullptr;
    const int parseFailed { ttparse (scanner, &final) };
    tt_delete_buffer (buffer, scanner);
    ttlex_destroy (scanner);

    if(parseFailed) {
        cout << "Parse error: " << extra.errorMessage << endl;
        return nullptr;
    } else {
        return shared_ptr<NProgram>(final);
    }
}

void printSet(pdb::PDBClient& pdbClient, std::string db, std::string set) {
    int count = 0;
    auto it = pdbClient.getSetIterator<TacoTensorBlock>(db, set);
    while(it->hasNextRecord()) {
        Handle<TacoTensorBlock> block = it->getNextRecord();

        TacoTensorBlockMeta& blockMeta = block->getKeyRef();

        std::cout << "BLOCK:  ";
        for(int i = 0; i != blockMeta.getBlock().size(); ++i) {
            std::cout << blockMeta.getBlock()[i] << " ";
        }
        std::cout << std::endl;

        if(!block->getValue().isNullPtr()) {
            TacoTensor& value = block->getValueRef();
            std::cout << "VALUE:  \n";
            std::cout << value.copyToTaco() << std::endl;
            std::cout << "DIAGNOSTICS:  \n";
            value.printDiagnostics();
        }

        count++;
    }
    std::cout << "COUNT:     " << count << std::endl;
}

void initEmpty(
    pdb::PDBClient &pdbClient,
    const std::string &set,
    std::vector<uint32_t> const& blockDim)
{
    uint32_t numBlocks = 1;
    for(int i = 0; i != blockDim.size(); ++i) {
        numBlocks *= blockDim[i];
    }

    uint32_t blockId = 0;
    std::vector<uint32_t> block;
    while(blockId != numBlocks) {
        const pdb::UseTemporaryAllocationBlock tempBlock{256 * 1024 * 1024};

        using VecBlock = Vector<Handle<TacoTensorBlock>>;

        // but each block here
        Handle<VecBlock> data = pdb::makeObject<VecBlock>();

        try {
            for(; blockId != numBlocks; ++blockId) {
                block.clear();
                auto index=blockId;
                for(size_t mode = 0; mode < blockDim.size()-1; mode++) {
                    block.push_back(index%blockDim[mode]);
                    index=index/blockDim[mode];
                }
                block.push_back(index);

                using Meta = TacoTensorBlockMeta;
                Handle<Meta> meta = makeObject<Meta>(block);

                data->push_back(makeObject<TacoTensorBlock>(nullptr, meta));
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

int main() {
    // make a client
    pdb::PDBClient pdbClient(8108, "localhost");

    // now, register a type for user data
    pdbClient.registerType("libraries/libArr.so");
    pdbClient.registerType("libraries/libTacoTensor.so");
    pdbClient.registerType("libraries/libTacoTensorBlock.so");
    pdbClient.registerType("libraries/libTacoTensorBlockMeta.so");
    pdbClient.registerType("libraries/libTacoAggregation.so");
    pdbClient.registerType("libraries/libTacoJoinAux.so");
    pdbClient.registerType("libraries/libTacoJoin2.so");
    pdbClient.registerType("libraries/libTacoJoin3.so");
    pdbClient.registerType("libraries/libTacoProjection.so");
    pdbClient.registerType("libraries/libTacoScanner.so");
    pdbClient.registerType("libraries/libTacoWriter.so");
    pdbClient.registerType("libraries/libTacoReadMTX.so");

    // these types are for TacoJoinAux used in TacoProjection and TacoJoinX
    pdbClient.registerType("libraries/libTAssignment.so");
    pdbClient.registerType("libraries/libTBinOp.so");
    pdbClient.registerType("libraries/libTExpr.so");
    pdbClient.registerType("libraries/libTMultOp.so");
    pdbClient.registerType("libraries/libTPlusOp.so");
    pdbClient.registerType("libraries/libTTensor.so");

    const pdb::UseTemporaryAllocationBlock tempBlock{256 * 1024 * 1024};

    // now, create a new database
    pdbClient.createDatabase("myData");

    // now, create the input and output sets
    pdbClient.createSet<TacoTensorBlock>("myData", "mat");
    pdbClient.createSet<TacoTensorBlock>("myData", "sum");

    //std::string mtxFile = "./applications/TestTaco/matrices/bcsstk17.mtx";
    std::string mtxFile = "./applications/TestTaco/matrices/lpi_galenet.mtx";
    std::vector<uint32_t> mtxDim({8,14});
    std::vector<uint32_t> blockDim({3, 3});
    initEmpty(pdbClient, "mat", blockDim);

    std::string programStr = "mat(i,j) = input;";
    programStr            += "sum() = mat(i,j);";
    programStr            += "output(sum);\0";

    NProgramPtr program = myParse(programStr);
    Handle<Computation> scanner = makeObject<TacoScanner>("myData", "mat");

    std::map<std::string, Handle<Computation>> inputs;
    inputs["mat"] = makeObject<TacoReadMTX>(
        mtxFile,
        mtxDim,
        blockDim);
    inputs["mat"]->setInput(0, scanner);

    std::vector<Handle<Computation>> outputs = program->compile("myData", inputs);

    pdbClient.executeComputations(outputs);

//    Handle<Computation> writer = makeObject<TacoWriter>("myData", "sum");
//    writer->setInput(0, inputs["mat"]);
//    pdbClient.executeComputations({writer});

    // get the set and print
    //printSet(pdbClient, "myData", "mat");
    printSet(pdbClient, "myData", "sum");

    // shutdown the server
    pdbClient.shutDownServer();

    std::cout << "-------------------------------------" << std::endl;

    // Now do the same thing in just taco.
    auto theMat = taco::readMTX(mtxFile, {taco::Dense, taco::Dense});
    //taco::Tensor<double> sum({(int)mtxDim[0]}, taco::Dense);
    taco::Tensor<double> sum({}, taco::Dense);
    taco::IndexVar i, j;
    //sum(i) = theMat(i,j);
    sum = theMat(i,j);

    sum.compile();
    sum.assemble();
    sum.compute();

    //std::cout << theMat << std::endl;
    std::cout << sum << std::endl;

    return 0;
}

