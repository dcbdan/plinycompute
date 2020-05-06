#include "tensorAlgebraDSL/headers/TLexer.h"
#include "tensorAlgebraDSL/headers/TParser.h"
#include "tensorAlgebraDSL/headers/TParserTypes.h"

#include "sharedLibraries/headers/TParserTypes.h"

#include <iostream>
#include <string>
#include <memory>

using std::cout;
using std::endl;
using std::cin;
using std::string;
using std::getline;


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
        std::cout  << "!!!!!!!!!!!!!!!!!!!" << std::endl;
        cout << "Parse error: " << extra.errorMessage << endl;
        return nullptr;
    } else {
        return shared_ptr<NProgram>(final);
    }
}

int main() {
    // parse input string and get the NAssignment
    std::string input = "A(i,j) = B(i,k)*C(k,j);\0";
    NProgramPtr program = myParse(input);
    std::cout << program->assignments.size() << std::endl;
    NAssignmentPtr nA = program->assignments[0];

    // get the pdb managed TAssignment object from the NAssignment
    Handle<TAssignment> tA = nA->createT();
    if(tA.isNullPtr()) {
        std::cout << "??????????#????\n";
        return 0;
    }

    // print out the answer using just taco
    taco::Dimension I, J, K;
    taco::TensorVar A("A", taco::Type(taco::Float32,{I,J}), {taco::Dense, taco::Dense});
    taco::TensorVar B("B", taco::Type(taco::Float32,{I,K}), {taco::Dense, taco::Dense});
    taco::TensorVar C("c", taco::Type(taco::Float32,{K,J}), {taco::Dense, taco::Dense});

    taco::IndexVar ii("i"), jj("j"), kk("k");
    taco::Assignment theAnswer = A(ii,jj) = B(ii,kk) * C(kk,jj);
    std::cout << theAnswer << std::endl;

    std::cout << "-------------------------------------------------------\n";

    // create a taco::Assignment object from TAssignment
    taco::Assignment tacoA = tA->getAssignment({A, B, C});
    std::cout << tacoA << std::endl;
}
