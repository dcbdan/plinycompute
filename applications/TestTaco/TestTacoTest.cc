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

void initEmpty(
    pdb::PDBClient &pdbClient,
    const std::string &db,
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
        pdbClient.sendData<TacoTensorBlock>(db, set, data);
    }
}

void registerTacoCompute(pdb::PDBClient& pdbClient) {
    // register types to do io, join, agg, projection
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
}

template <typename SetIterator>
bool testEqual(SetIterator iter, taco::TensorBase& tensor) {
    // It isn't trivial to check that the two outputs are exactly equal.
    // So what this function will do is just take the sum of all elements
    // of both and test that those values are equal.
    auto getSum = [](taco::TensorBase& v) {
        taco::TensorBase scalar(taco::type<double>(), {}, taco::Dense);
        std::vector<taco::IndexVar> indices(v.getOrder());
        scalar() = v(indices);

        scalar.compile();
        scalar.assemble();
        scalar.compute();

        return ((double*)scalar.getTacoTensorT()->vals)[0];
    };

    double overall = getSum(tensor);

    // go through each block and add all the elements
    double agg = 0.0;
    while(iter->hasNextRecord()) {

        Handle<TacoTensorBlock> block = iter->getNextRecord();
        Handle<TacoTensor> blockTensorPtr = block->getValue();
        if(blockTensorPtr.isNullPtr()) {
            std::cout << "Warning: iter contains null value" << std::endl;
            return false;
        }
        TacoTensor& blockTensor = *blockTensorPtr;
        taco::TensorBase blockTensorCopy = blockTensor.copyToTaco();
        agg += getSum(blockTensorCopy);
    }

    return (agg - overall)*(agg-overall) < 0.00001;
}

int main() {
    // make a client
    pdb::PDBClient pdbClient(8108, "localhost");

    registerTacoCompute(pdbClient);

    const pdb::UseTemporaryAllocationBlock tempBlock{256 * 1024 * 1024};

    std::string const db = "TestTaco";
    // now, create a new database
    pdbClient.createDatabase(db);

    // Test 01
    //   rowsum(i) = mat(i,j)
    //   colsum(j) = mat(i,j)
    //   sum()     = mat(i,j)
    {
        // now, create the input and output sets
        pdbClient.createSet<TacoTensorBlock>(db, "mat01");
        pdbClient.createSet<TacoTensorBlock>(db, "rowsum01");
        pdbClient.createSet<TacoTensorBlock>(db, "colsum01");
        pdbClient.createSet<TacoTensorBlock>(db, "sum01");


        //std::string mtxFile = "./applications/TestTaco/matrices/lpi_galenet.mtx";
        //std::vector<uint32_t> mtxDim({8,14});
        //std::vector<uint32_t> blockDim({2, 3});
        std::string mtxFile = "./applications/TestTaco/matrices/bcsstk17.mtx";
        std::vector<uint32_t> mtxDim({10974, 10974});
        std::vector<uint32_t> blockDim({33, 37});
        initEmpty(pdbClient, db, "mat01", blockDim);

        std::string programStr = "";
        programStr += "mat01(i,j) = input;";
        programStr += "rowsum01(i) = mat01(i,j);";
        programStr += "colsum01(j) = mat01(i,j);";
        programStr += "sum01()     = mat01(i,j);";
        programStr += "output(rowsum01); output(colsum01); output(sum01);";
        programStr += "\0";

        NProgramPtr program = myParse(programStr);
        Handle<Computation> scanner = makeObject<TacoScanner>(db, "mat01");

        std::map<std::string, Handle<Computation>> inputs;
        inputs["mat01"] = makeObject<TacoReadMTX>(mtxFile, mtxDim, blockDim);
        inputs["mat01"]->setInput(0, scanner);

        std::vector<Handle<Computation>> outputs = program->compile(db, inputs);
        pdbClient.executeComputations(outputs);

        taco::TensorBase mat01 = taco::readMTX(mtxFile, {taco::Dense, taco::Sparse});
        taco::TensorBase sum01(taco::type<double>(), {}, taco::Dense);
        taco::TensorBase rowsum01(taco::type<double>(), {(int)mtxDim[0]}, taco::Dense);
        taco::TensorBase colsum01(taco::type<double>(), {(int)mtxDim[1]}, taco::Dense);

        taco::IndexVar i,j;
        sum01       = mat01(i,j);
        rowsum01(i) = mat01(i,j);
        colsum01(j) = mat01(i,j);

        sum01.compile();    sum01.assemble();    sum01.compute();
        rowsum01.compile(); rowsum01.assemble(); rowsum01.compute();
        colsum01.compile(); colsum01.assemble(); colsum01.compute();

        auto itS01 = pdbClient.getSetIterator<TacoTensorBlock>(db, "sum01");
        auto itR01 = pdbClient.getSetIterator<TacoTensorBlock>(db, "rowsum01");
        auto itC01 = pdbClient.getSetIterator<TacoTensorBlock>(db, "colsum01");

        std::cout << "Test01: ";
        std::cout << (testEqual(itS01, sum01)    ? "pass" : "fail") << " ";
        std::cout << (testEqual(itR01, rowsum01) ? "pass" : "fail") << " ";
        std::cout << (testEqual(itC01, colsum01) ? "pass" : "fail") << std::endl;
    }

    //// Test 02
    ////   tmp(i,j) = B(i,k) * C(k,j);
    ////   A(i,j)   = tmp(i,j) + d(j);
    //{
    //    pdbClient.createSet<TacoTensorBlock>(db, "A02");
    //    pdbClient.createSet<TacoTensorBlock>(db, "B02");
    //    pdbClient.createSet<TacoTensorBlock>(db, "C02");
    //    pdbClient.createSet<TacoTensorBlock>(db, "d02");
    //}


    // shutdown the server
    pdbClient.shutDownServer();

    return 0;
}
