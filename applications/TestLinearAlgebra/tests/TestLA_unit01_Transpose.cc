/*****************************************************************************
 *                                                                           *
 *  Copyright 2018 Rice University                                           *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *      http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 *****************************************************************************/
#ifndef TEST_LA_01_CC
#define TEST_LA_01_CC


// by Binhang, May 2017
// to test matrix transpose implemented by selection;
#include <ctime>
#include <chrono>

#include "PDBDebug.h"
#include "PDBString.h"
#include "Lambda.h"
#include "PDBClient.h"
#include "LAScanMatrixBlockSet.h"
#include "LAWriteMatrixBlockSet.h"
#include "MatrixBlock.h"
#include "LATransposeSelection.h"


using namespace pdb;
int main(int argc, char* argv[]) {
    bool printResult = true;
    std::cout << "Usage: #printResult[Y/N] #managerIp #addData[Y/N]" << std::endl;
    if (argc > 1) {
        if (strcmp(argv[1], "N") == 0) {
            printResult = false;
            std::cout << "You successfully disabled printing result." << std::endl;
        } else {
            printResult = true;
            std::cout << "Will print result." << std::endl;
        }
    } else {
        std::cout << "Will print result. If you don't want to print result, you can add N as the "
                     "first parameter to disable result printing." << std::endl;
    }

    std::string managerIp = "localhost";
    if (argc > 2) {
        managerIp = argv[2];
    }
    std::cout << "Manager IP Address is " << managerIp << std::endl;

    bool whetherToAddData = true;
    if (argc > 3) {
        if (strcmp(argv[3], "N") == 0) {
            whetherToAddData = false;
        }
    }
    PDBClient pdbClient(8108, managerIp);

    int blockSize = 64;  // Force it to be 64 by now.
    std::cout << "To add data with size: " << blockSize << "MB" << std::endl;

    if (whetherToAddData) {
        // Step 1. Create Database and Set
        // now, register a type for user data
        pdbClient.registerType("libraries/libMatrixBlock.so");
        // now, create a new database
        pdbClient.createDatabase("LA01_db");
        // now, create a new set in that database
        pdbClient.createSet<MatrixBlock>("LA01_db", "LA_input_set");

        // Step 2. Add data
        int matrixRowNums = 4;
        int matrixColNums = 4;
        int blockRowNums = 10;
        int blockColNums = 5;
        int total = 0;
        // Add Matrix
        pdb::makeObjectAllocatorBlock(blockSize * 1024 * 1024, true);
        pdb::Handle<pdb::Vector<pdb::Handle<MatrixBlock>>> storeMatrix =
                pdb::makeObject<pdb::Vector<pdb::Handle<MatrixBlock>>>();

        for (int i = 0; i < matrixRowNums; i++) {
            for (int j = 0; j < matrixColNums; j++) {
                pdb::Handle<MatrixBlock> myData =
                        pdb::makeObject<MatrixBlock>(i, j, blockRowNums, blockColNums);
                // Foo initialization
                for (int ii = 0; ii < blockRowNums; ii++) {
                    for (int jj = 0; jj < blockColNums; jj++) {
                        if (i < j) {
                            (*(myData->getRawDataHandle()))[ii * blockColNums + jj] =
                                    (ii <= jj ? 1.0 : 0.0);
                        } else if (i > j) {
                            (*(myData->getRawDataHandle()))[ii * blockColNums + jj] =
                                    (ii >= jj ? 1.0 : 0.0);
                        } else {
                            (*(myData->getRawDataHandle()))[ii * blockColNums + jj] =
                                    (ii == jj ? 1.0 : 0.0);
                        }
                    }
                }
                std::cout << "New block: " << total << std::endl;
                myData->print();
                storeMatrix->push_back(myData);
                total++;
            }
        }
        pdbClient.sendData<MatrixBlock>("LA01_db", "LA_input_set", storeMatrix);
        PDB_COUT << total << " MatrixBlock data sent to dispatcher server~~" << std::endl;
    }
    // now, create a new set in that database to store output data

    PDB_COUT << "to create a new set for storing output data" << std::endl;
    pdbClient.createSet<MatrixBlock>("LA01_db", "LA_transpose_set");

    // Step 3. To execute a Query
    // for allocations
    const UseTemporaryAllocationBlock tempBlock{1024 * 1024 * 128};

    // register this query class
    pdbClient.registerType("libraries/libLATransposeSelection.so");
    pdbClient.registerType("libraries/libLAScanMatrixBlockSet.so");
    pdbClient.registerType("libraries/libLAWriteMatrixBlockSet.so");

    Handle<Computation> myScanSet = makeObject<LAScanMatrixBlockSet>("LA01_db", "LA_input_set");
    Handle<Computation> myQuery = makeObject<LATransposeSelection>();
    myQuery->setInput(myScanSet);

    Handle<Computation> myWriteSet =makeObject<LAWriteMatrixBlockSet>("LA01_db", "LA_transpose_set");
    myWriteSet->setInput(myQuery);

    auto begin = std::chrono::high_resolution_clock::now();

    pdbClient.executeComputations({myWriteSet});

    std::cout << std::endl;

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << std::endl;
    // print the results
    if (printResult) {

        std::cout << "to print result..." << std::endl;
        auto input_iter = pdbClient.getSetIterator<MatrixBlock>("LA01_db", "LA_input_set");
        std::cout << "Query input: " << std::endl;
        int countIn = 0;
        while(input_iter->hasNextRecord()){
            countIn++;
            std::cout << countIn << ":";
            auto r = input_iter->getNextRecord();
            r->print();
            std::cout << std::endl;
        }
        std::cout << "Matrix input block nums:" << countIn << std::endl;

        auto output_iter = pdbClient.getSetIterator<MatrixBlock>("LA01_db", "LA_transpose_set");
        std::cout << "Transpose query results: " << std::endl;
        int countOut = 0;
        while(output_iter->hasNextRecord()){
            countOut++;
            std::cout << countOut << ":";
            auto r = output_iter->getNextRecord();
            r->print();

            std::cout << std::endl;
        }
        std::cout << "Transpose output count:" << countOut << "\n";
    }

    pdbClient.removeSet("LA01_db", "LA_transpose_set");

    int code = system("scripts/cleanupSoFiles.sh force");
    if (code < 0) {
        std::cout << "Can't cleanup so files" << std::endl;
    }
    std::cout << "Time Duration: "
              << std::chrono::duration_cast<std::chrono::duration<float>>(end - begin).count()
              << " secs." << std::endl;
}

#endif
