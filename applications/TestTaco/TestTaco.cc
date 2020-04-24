#include <boost/program_options.hpp>
#include <PDBClient.h>
#include <GenericWork.h>

#include "taco.h"

// for assignment -> stmt
#include <taco/index_notation/transformations.h>
// for lower: stmt -> stmt to then put in Module
#include "taco/lower/lower.h"

#include "sharedLibraries/headers/TacoTensor.h"
#include "sharedLibraries/headers/Module.h"

using namespace taco;
using namespace taco::ir;
using namespace pdb;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    // Create formats
    Format csr({Dense, Sparse});
    Format csf({Sparse, Sparse, Sparse});
    Format sv({Sparse});

    // Create tensors
    //Tensor<double> A({2, 3}, csr);
    //Tensor<double> A({2, 3}, {Dense, Dense});
    Tensor<double> A({2, 3}, {Sparse, Sparse});
    Tensor<double> B({2, 3, 4}, csf);
    Tensor<double> c({4}, sv);

    // Insert data into B and c
    B.insert({0, 0, 0}, 1.0);
    B.insert({1, 2, 0}, 2.0);
    B.insert({1, 2, 1}, 3.0);
    c.insert({0}, 4.0);
    c.insert({1}, 5.0);

    // Pack data as described by the formats
    B.pack();
    c.pack();

    IndexVar i("i"), j("j"), k("k");
    IndexVar ll("i"), mm("j"), nn("k");

    // The type of this whole assignment is Assignment
    Assignment assignment  = A(i, j) = B(i, j, k) * c(k);

    int nMegabyte = 1000;
    makeObjectAllocatorBlock(nMegabyte * 1024 * 1024, true);

    Handle<TacoTensor> pdbA = makeObject<TacoTensor>(
      taco::type<double>(),
      std::vector<int>{2, 3},
      A.getFormat());

    Handle<TacoTensor> pdbB = makeObject<TacoTensor>(taco::type<double>(), B.getTacoTensorT());
    Handle<TacoTensor> pdbc = makeObject<TacoTensor>(taco::type<double>(), c.getTacoTensorT());

    pdb::TacoModule tm;
    void* tmOut = tm[assignment];
    TacoTensor::callKernel(tmOut, {pdbA, pdbB, pdbc});

    std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";

    std::cout << (pdbA->copyToTaco()) << std::endl;
    std::cout << (pdbB->copyToTaco()) << std::endl;
    std::cout << (pdbc->copyToTaco()) << std::endl;

    std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";

    A.evaluate();
    std::cout << A << std::endl;
    std::cout << B << std::endl;
    std::cout << c << std::endl;
}
