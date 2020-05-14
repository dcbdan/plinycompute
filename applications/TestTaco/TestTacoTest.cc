#include <PDBClient.h>

#include "sharedLibraries/headers/TacoTensor.h"
#include "sharedLibraries/headers/TacoTensorBlock.h"
#include "sharedLibraries/headers/TacoTensorBlockMeta.h"

#include "sharedLibraries/headers/BlockOffsetter.h"

#include "sharedLibraries/headers/TacoReadMTX.h"

#include "tensorAlgebraDSL/headers/NLexer.h"
#include "tensorAlgebraDSL/headers/NParser.h"
#include "tensorAlgebraDSL/headers/NParserTypes.h"

#include <string>
#include <random>
#include <iostream>
#include <vector>

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
    pdbClient.registerType("libraries/libTUnOp.so");
    pdbClient.registerType("libraries/libTExpr.so");
    pdbClient.registerType("libraries/libTMultOp.so");
    pdbClient.registerType("libraries/libTPlusOp.so");
    pdbClient.registerType("libraries/libTAbsOp.so");
    pdbClient.registerType("libraries/libTMinOp.so");
    pdbClient.registerType("libraries/libTMaxOp.so");
    pdbClient.registerType("libraries/libTSubtractOp.so");
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
        std::cout << "BLOCK: ";
        for(int i = 0; i != block->getKey()->getBlock().size(); ++i) {
            std::cout << block->getKey()->getBlock()[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "VALUE: " << blockTensorCopy << std::endl;
        agg += getSum(blockTensorCopy);
    }

    std::cout << "ORACLE: " << tensor << std::endl;

    return (agg - overall) * (agg - overall) < 0.00001;
}

template <typename Int>
struct IterateCoord {
    IterateCoord(std::vector<Int> begIn, std::vector<Int> endIn)
        : beg(begIn), end(endIn)
    {}

    bool getNext(std::vector<Int>& coord) const {
        while(true) {
            coord.back() += 1;
            if(coord.back() == end[coord.size()-1]) {
                coord.pop_back();
            } else {
                break;
            }

            if(coord.size() == 0) {
                return false;
            }
        }

        auto order = beg.size();
        while(coord.size() != order) {
            coord.push_back(beg[coord.size()]);
        }

        return true;
    }

private:
    std::vector<Int> beg;
    std::vector<Int> end;
};

// This function creates two equal tensors with random values
// The first in ret, in just taco. The second in (db,set) in
// the form of a TacoTensorBlock
void initRandomTensor(
    std::vector<uint32_t> const& blockDim,
    std::vector<uint32_t> const& dim,
    double const& sparsity,
    std::string const& db,
    std::string const& set,
    pdb::PDBClient& pdbClient,
    taco::TensorBase& ret)
{
    auto order = blockDim.size();

    // random generator stuff
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution rbern(1 - sparsity);
    std::normal_distribution<double> rnorm;

    auto genIsNonZero = [&]() {
        return rbern(gen);
    };
    auto genVal = [&]() {
        return rnorm(gen);
    };

    // fill out block and ret with random values..
    // offsetter is coordinate block coord and global coord
    auto fill = [&](
        taco::TensorBase& block,
        BlockOffsetter offsetter)
    {
        std::vector<int> beg, end;
        offsetter.getRange(beg, end);
        std::vector<int> coord = beg;

        IterateCoord<int> iter(beg, end);
        do {
            // maybe add a value
            bool isNonZero = genIsNonZero();
            if(isNonZero) {
                double val = genVal();
                ret.insert(coord, val);
                block.insert(offsetter.getCoordInBlock(coord), val);
            }
        } while(iter.getNext(coord));

    };

    std::vector<uint32_t> blockBeg(order, 0);
    std::vector<uint32_t> blockCoord = blockBeg;
    IterateCoord<uint32_t> iterBlock(blockBeg, blockDim);

    bool hasNext = true;
    while(hasNext) {
        const pdb::UseTemporaryAllocationBlock tempBlock{256 * 1024 * 1024};

        using VecBlock = Vector<Handle<TacoTensorBlock>>;

        // but each block here
        Handle<VecBlock> data = pdb::makeObject<VecBlock>();

        try {
            do {
                BlockOffsetter offsetter(dim, blockDim, blockCoord);

                // create a dense block
                taco::TensorBase block(
                    taco::type<double>(),
                    offsetter.getDimensionOfBlock(),
                    std::vector<taco::ModeFormatPack>(order, taco::dense));

                // fill it (and ret)
                fill(block, offsetter);

                // done insetering to block, so pack
                block.pack();

                // now copy to pdb
                using Meta = TacoTensorBlockMeta;
                Handle<Meta>       key   = makeObject<Meta>(blockCoord);
                Handle<TacoTensor> value = makeObject<TacoTensor>(block);
                data->push_back(makeObject<TacoTensorBlock>(value, key));

                hasNext = iterBlock.getNext(blockCoord);
            } while(hasNext);
        } catch(pdb::NotEnoughSpace &n) {
        }

        // init the records
        getRecord(data);

        // send the data
        pdbClient.sendData<TacoTensorBlock>(db, set, data);
    }

    // done adding to ret
    ret.pack();
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

    // Test 02
    //   tmp(i,j) = B(i,k) * C(k,j);
    //   A(i,j)   = tmp(i,j) + D(i,j);
    {
        pdbClient.createSet<TacoTensorBlock>(db, "A02");
        pdbClient.createSet<TacoTensorBlock>(db, "B02");
        pdbClient.createSet<TacoTensorBlock>(db, "C02");
        pdbClient.createSet<TacoTensorBlock>(db, "D02");

        uint32_t I = 5;
        uint32_t J = 10;
        uint32_t K = 15;
        uint32_t bI = 1;
        uint32_t bJ = 1;
        uint32_t bK = 1;
        int iI = I; // ...
        int iJ = J;
        int iK = K;

        taco::TensorBase A02(taco::type<double>(), {iI,iJ}, {taco::dense, taco::dense});
        // no init! will be computed

        taco::TensorBase B02(taco::type<double>(), {iI,iK}, {taco::dense, taco::dense});
        initRandomTensor({bI,bK}, {I,K}, 0.5, db, "B02", pdbClient, B02);

        taco::TensorBase C02(taco::type<double>(), {iK,iJ}, {taco::dense, taco::dense});
        initRandomTensor({bK,bJ}, {K,J}, 0.5, db, "C02", pdbClient, C02);

        taco::TensorBase D02(taco::type<double>(), {iI,iJ}, {taco::dense, taco::dense});
        initRandomTensor({bI,bJ}, {I,J}, 0.5, db, "D02", pdbClient, D02);

        std::string programStr = "";
        programStr += "B02(i,k) = input; C02(k,j) = input; D02(i,j)=input;";
        programStr += "tmp(i,j) = B02(i,k) * C02(k,j);";
        programStr += "A02(i,j) = tmp(i,j) + D02(i,j);";
        programStr += "output(A02);";
        programStr += "\0";

        NProgramPtr program = myParse(programStr);
        std::map<std::string, Handle<Computation>> inputs;
        inputs["B02"] = makeObject<TacoScanner>(db, "B02");
        inputs["C02"] = makeObject<TacoScanner>(db, "C02");
        inputs["D02"] = makeObject<TacoScanner>(db, "D02");

        std::vector<Handle<Computation>> outputs = program->compile(db, inputs);
        pdbClient.executeComputations(outputs);

        taco::IndexVar i,j,k;
        A02(i,j) = B02(i,k) * C02(k,j) + D02(i,j);

        A02.evaluate();

        auto itA02 = pdbClient.getSetIterator<TacoTensorBlock>(db, "A02");

        std::cout << "Test02: ";
        std::cout << (testEqual(itA02, A02)    ? "pass" : "fail") << std::endl;
    }

    // Test 03
    //   A(i,j,k)   = B(i,j,k) + C(i,j,k);
    {
        pdbClient.createSet<TacoTensorBlock>(db, "A03");
        pdbClient.createSet<TacoTensorBlock>(db, "B03");
        pdbClient.createSet<TacoTensorBlock>(db, "C03");

        uint32_t I = 5;
        uint32_t J = 10;
        uint32_t K = 15;
        uint32_t bI = 1;
        uint32_t bJ = 1;
        uint32_t bK = 1;
        int iI = I; // ...
        int iJ = J;
        int iK = K;

        taco::TensorBase A03(taco::type<double>(), {iI,iJ,iK}, {taco::dense, taco::dense, taco::dense});
        // no init! will be computed

        taco::TensorBase B03(taco::type<double>(), {iI,iJ,iK}, {taco::dense, taco::dense, taco::dense});
        initRandomTensor({bI,bJ,bK}, {I,J,K}, 0.5, db, "B03", pdbClient, B03);

        taco::TensorBase C03(taco::type<double>(), {iI,iJ,iK}, {taco::dense, taco::dense, taco::dense});
        initRandomTensor({bI,bJ,bK}, {I,J,K}, 0.5, db, "C03", pdbClient, C03);

        std::string programStr = "";
        programStr += "B03(i,j,k) = input; C03(i,j,k) = input;";
        programStr += "A03(i,j,k) = B03(i,j,k) + C03(i,j,k);";
        programStr += "output(A03);";
        programStr += "\0";

        NProgramPtr program = myParse(programStr);
        std::map<std::string, Handle<Computation>> inputs;
        inputs["B03"] = makeObject<TacoScanner>(db, "B03");
        inputs["C03"] = makeObject<TacoScanner>(db, "C03");
        inputs["D03"] = makeObject<TacoScanner>(db, "D03");

        std::vector<Handle<Computation>> outputs = program->compile(db, inputs);
        pdbClient.executeComputations(outputs);

        taco::IndexVar i,j,k;
        A03(i,j,k) = B03(i,j,k) + C03(i,j,k);

        A03.evaluate();

        auto itA03 = pdbClient.getSetIterator<TacoTensorBlock>(db, "A03");

        std::cout << "Test03: ";
        std::cout << (testEqual(itA03, A03)    ? "pass" : "fail") << std::endl;
    }

    // Test 04
    //   A(i)   = B(i) * C(i);
    {
        pdbClient.createSet<TacoTensorBlock>(db, "A04");
        pdbClient.createSet<TacoTensorBlock>(db, "B04");
        pdbClient.createSet<TacoTensorBlock>(db, "C04");

        uint32_t I = 113;
        uint32_t bI = 13;
        int iI = I; // ...

        taco::TensorBase A04(taco::type<double>(), {iI}, {taco::dense});
        // no init! will be computed

        taco::TensorBase B04(taco::type<double>(), {iI}, {taco::dense});
        initRandomTensor({bI}, {I}, 0.5, db, "B04", pdbClient, B04);

        taco::TensorBase C04(taco::type<double>(), {iI}, {taco::dense});
        initRandomTensor({bI}, {I}, 0.5, db, "C04", pdbClient, C04);

        std::string programStr = "";
        programStr += "B04(i) = input; C04(i) = input;";
        programStr += "A04(i) = B04(i) + C04(i);";
        programStr += "output(A04);";
        programStr += "\0";

        NProgramPtr program = myParse(programStr);
        std::map<std::string, Handle<Computation>> inputs;
        inputs["B04"] = makeObject<TacoScanner>(db, "B04");
        inputs["C04"] = makeObject<TacoScanner>(db, "C04");

        std::vector<Handle<Computation>> outputs = program->compile(db, inputs);
        pdbClient.executeComputations(outputs);

        taco::IndexVar i,j,k;
        A04(i) = B04(i) + C04(i);

        A04.evaluate();

        auto itA04 = pdbClient.getSetIterator<TacoTensorBlock>(db, "A04");

        std::cout << "Test04: ";
        std::cout << (testEqual(itA04, A04)    ? "pass" : "fail") << std::endl;
    }

    // shutdown the server
    pdbClient.shutDownServer();

    return 0;
}
