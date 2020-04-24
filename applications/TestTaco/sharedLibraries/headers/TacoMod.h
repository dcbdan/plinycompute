//#include "taco.h"

#include "taco/index_notation/index_notation.h"
#include "taco/index_notation/index_notation_nodes.h"
#include "taco/index_notation/index_notation_printer.h"
#include "taco/index_notation/index_notation_rewriter.h"

namespace tacomod {

class AssignmentNotationPrinter : public taco::IndexNotationPrinter {
public:
    using taco::IndexNotationPrinter::print;
    using taco::IndexNotationPrinter::visit;

    AssignmentNotationPrinter(std::ostream& os)
        : IndexNotationPrinter(os), os(os) {
    }
    void visit(const taco::AccessNode* op) {
      os << op->tensorVar.getName();
      // Instead of printing out the index variables, print out
      // the Type
      ////if (op->indexVars.size() > 0) {
      ////  os << "(" << taco::util::join(op->indexVars,",") << ")";
      ////}
      os << "(" << op->tensorVar.getType() << ")";
    }
private:
    std::ostream& os;
};

// Assignment statements that were created with different tensorVars and
// indexVars can be compared as equal using the code below.
// See:
//    int main() {
//        Dimension M, N, O;
//
//        IndexVar i("i"), j("j"), k("k"), l("l");
//
//        TensorVar A("A", Type(Float32,{M,N}),   CSR);
//        TensorVar B("B", Type(Float32,{M,N,O}), {Dense, Dense, Sparse});
//        TensorVar c("c", Type(Float32,{O}),     Dense);
//
//        std::cout << A << std::endl;
//        std::cout << B << std::endl;
//        std::cout << c << std::endl;
//
//        // The type of this whole assignment is Assignment
//        Assignment assignment = A(i,j) = B(i,j,k) * c(k);
//
//        Dimension MM, NN, OO;
//
//        IndexVar ii("a"), jj("b"), kk("c");
//
//        TensorVar AA("A", Type(Float32,{MM,MM}),   CSR);
//        TensorVar BB("B", Type(Float32,{MM,MM,MM}), {Dense, Dense, Sparse});
//        TensorVar cc("c", Type(Float32,{MM}),     Dense);
//
//        //Assignment assignment2 = A(ii,jj) = B(ii,jj,kk) * c(kk);
//        Assignment assignment2 = AA(j,i) = BB(j,i,l) * cc(l);
//
//        std::cout << assignment << std::endl << assignment2 << std::endl;
//
//        // Rewrite IndexVars
//        RewriteIndexVars r1;
//        Assignment a1 = to<Assignment>(r1.rewrite(assignment));
//        RewriteIndexVars r2(r1.storedIndexVars);
//        Assignment a2 = to<Assignment>(r2.rewrite(assignment2));
//
//        std::cout << "------------------" << std::endl;
//        std::cout << a1 << std::endl;
//        std::cout << a2 << std::endl;
//        std::cout << "Are they equal now? " << equalsMod(a1, a2) << std::endl;
//        std::cout << "------------------" << std::endl;
//     }
//
// using namespace taco;
//
// bool equalsMod(IndexExpr a, IndexExpr b);
// bool equalsMod(IndexStmt a, IndexStmt b);
//
// // taco/index_notation/index_notation.cpp defines an
// // Equals struct and a equals function that makes it possible
// // to compare taco::Assignment objects but they require the underlying
// // TensorVar objects to be exactly the same.
// //
// // This struct navigates the computational graph and the visit
// // functions below check that that the nodes are equal.
// // Here is what I've changed for the original Equals struct:
// //   1. visit(const AccessNode*) now compares TensorVars by checking
// //      that they have the same format and type
// //   2. instead of calling equals in the recursion, I'm calling equalsMod
// struct EqualsMod : public IndexNotationVisitorStrict {
//   bool eq = false;
//   IndexExpr bExpr;
//   IndexStmt bStmt;
//
//   bool check(IndexExpr a, IndexExpr b) {
//     this->bExpr = b;
//     a.accept(this);
//     return eq;
//   }
//
//   bool check(IndexStmt a, IndexStmt b) {
//     this->bStmt = b;
//     a.accept(this);
//     return eq;
//   }
//
//   using IndexNotationVisitorStrict::visit;
//
//   void visit(const AccessNode* anode) {
//     if (!isa<AccessNode>(bExpr.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<AccessNode>(bExpr.ptr);
//     if (anode->indexVars.size() != anode->indexVars.size()) {
//       eq = false;
//       return;
//     }
//
//     // The tensorVars should be equal if they have the same type and format
//     // (not if they point to the same tensorVar object)
//     if (anode->tensorVar.getType()   != bnode->tensorVar.getType() ||
//         anode->tensorVar.getFormat() != bnode->tensorVar.getFormat()) {
//       eq = false;
//       return;
//     }
//
//     // The indexVars are only equal if they are indeed the same indexVars.
//     for (size_t i = 0; i < anode->indexVars.size(); i++) {
//       if (anode->indexVars[i] != bnode->indexVars[i]) {
//         eq = false;
//         return;
//       }
//     }
//     eq = true;
//   }
//
//   void visit(const LiteralNode* anode) {
//     if (!isa<LiteralNode>(bExpr.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<LiteralNode>(bExpr.ptr);
//     if (anode->getDataType() != bnode->getDataType()) {
//       eq = false;
//       return;
//     }
//     if (memcmp(anode->val,bnode->val,anode->getDataType().getNumBytes()) != 0) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
//
//   template <class T>
//   bool unaryEquals(const T* anode, IndexExpr b) {
//     if (!isa<T>(b.ptr)) {
//       return false;
//     }
//     auto bnode = to<T>(b.ptr);
//     if (!equalsMod(anode->a, bnode->a)) {
//       return false;
//     }
//     return true;
//   }
//
//   void visit(const NegNode* anode) {
//     eq = unaryEquals(anode, bExpr);
//   }
//
//   void visit(const SqrtNode* anode) {
//     eq = unaryEquals(anode, bExpr);
//   }
//
//   template <class T>
//   bool binaryEquals(const T* anode, IndexExpr b) {
//     if (!isa<T>(b.ptr)) {
//       return false;
//     }
//     auto bnode = to<T>(b.ptr);
//     if (!equalsMod(anode->a, bnode->a) || !equalsMod(anode->b, bnode->b)) {
//       return false;
//     }
//     return true;
//   }
//
//   void visit(const AddNode* anode) {
//     eq = binaryEquals(anode, bExpr);
//   }
//
//   void visit(const SubNode* anode) {
//     eq = binaryEquals(anode, bExpr);
//   }
//
//   void visit(const MulNode* anode) {
//     eq = binaryEquals(anode, bExpr);
//   }
//
//   void visit(const DivNode* anode) {
//     eq = binaryEquals(anode, bExpr);
//   }
//
//   void visit(const CastNode* anode) {
//     if (!isa<CastNode>(bExpr.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<CastNode>(bExpr.ptr);
//     if (anode->getDataType() != bnode->getDataType() ||
//         !equalsMod(anode->a, bnode->a)) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
//
//   void visit(const CallIntrinsicNode* anode) {
//     if (!isa<CallIntrinsicNode>(bExpr.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<CallIntrinsicNode>(bExpr.ptr);
//     if (anode->func->getName() != bnode->func->getName() ||
//         anode->args.size() != bnode->args.size()) {
//       eq = false;
//       return;
//     }
//     for (size_t i = 0; i < anode->args.size(); ++i) {
//       if (!equalsMod(anode->args[i], bnode->args[i])) {
//         eq = false;
//         return;
//       }
//     }
//     eq = true;
//   }
//
//   void visit(const ReductionNode* anode) {
//     if (!isa<ReductionNode>(bExpr.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<ReductionNode>(bExpr.ptr);
//     if (!(equalsMod(anode->op, bnode->op) && equalsMod(anode->a, bnode->a))) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
//
//   void visit(const AssignmentNode* anode) {
//     if (!isa<AssignmentNode>(bStmt.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<AssignmentNode>(bStmt.ptr);
//     if (!equalsMod(anode->lhs, bnode->lhs) || !equalsMod(anode->rhs, bnode->rhs) ||
//         !equalsMod(anode->op, bnode->op)) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
//
//   void visit(const YieldNode* anode) {
//     if (!isa<YieldNode>(bStmt.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<YieldNode>(bStmt.ptr);
//     if (anode->indexVars.size() != anode->indexVars.size()) {
//       eq = false;
//       return;
//     }
//     for (size_t i = 0; i < anode->indexVars.size(); i++) {
//       if (anode->indexVars[i] != bnode->indexVars[i]) {
//         eq = false;
//         return;
//       }
//     }
//     if (!equalsMod(anode->expr, bnode->expr)) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
//
//   void visit(const ForallNode* anode) {
//     if (!isa<ForallNode>(bStmt.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<ForallNode>(bStmt.ptr);
//     if (anode->indexVar != bnode->indexVar ||
//         !equalsMod(anode->stmt, bnode->stmt) ||
//         anode->tags != bnode->tags) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
//
//   void visit(const WhereNode* anode) {
//     if (!isa<WhereNode>(bStmt.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<WhereNode>(bStmt.ptr);
//     if (!equalsMod(anode->consumer, bnode->consumer) ||
//         !equalsMod(anode->producer, bnode->producer)) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
//
//   void visit(const SequenceNode* anode) {
//     if (!isa<SequenceNode>(bStmt.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<SequenceNode>(bStmt.ptr);
//     if (!equalsMod(anode->definition, bnode->definition) ||
//         !equalsMod(anode->mutation, bnode->mutation)) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
//
//   void visit(const MultiNode* anode) {
//     if (!isa<MultiNode>(bStmt.ptr)) {
//       eq = false;
//       return;
//     }
//     auto bnode = to<MultiNode>(bStmt.ptr);
//     if (!equalsMod(anode->stmt1, bnode->stmt1) ||
//         !equalsMod(anode->stmt2, bnode->stmt2)) {
//       eq = false;
//       return;
//     }
//     eq = true;
//   }
// };
//
// bool equalsMod(IndexExpr a, IndexExpr b) {
//   if (!a.defined() && !b.defined()) {
//     return true;
//   }
//   if ((a.defined() && !b.defined()) || (!a.defined() && b.defined())) {
//     return false;
//   }
//   return EqualsMod().check(a,b);
// }
//
//
// bool equalsMod(IndexStmt a, IndexStmt b) {
//   if (!a.defined() && !b.defined()) {
//     return true;
//   }
//   if ((a.defined() && !b.defined()) || (!a.defined() && b.defined())) {
//     return false;
//   }
//   return EqualsMod().check(a,b);
// }
//
//
// class RewriteIndexVars : public IndexNotationRewriter {
//     int count;
// public:
//     std::vector<IndexVar> storedIndexVars;
//     std::map<IndexVar, int> toWhere;
//
//     RewriteIndexVars(): count(0) {}
//     RewriteIndexVars(std::vector<IndexVar> const& storedIndexVars)
//         : count(0), storedIndexVars(storedIndexVars)
//     {}
//
// private:
//     // to get rid of the overloaded virtual warning
//     using IndexNotationRewriter::visit;
//
//     void visit(const AssignmentNode* op) {
//         // rewrite the right side
//         IndexExpr rhs = rewrite(op->rhs);
//
//         // rewrite the left side
//         op->lhs.accept(this);
//         Access lhs = to<Access>(expr);
//
//         // create a new one
//         stmt = new AssignmentNode(lhs, rhs, op->op);
//     }
//
//     void visit(const AccessNode* op) {
//         // Rewrite all index vars of an access node
//
//         std::vector<IndexVar> newIndexVars;
//         newIndexVars.reserve(op->indexVars.size());
//         for(auto& i: op->indexVars) {
//             if(!util::contains(toWhere, i)) {
//                 if((int)(storedIndexVars.size()) == count) {
//                     storedIndexVars.push_back(i);//emplace_back(i.getName());
//                 }
//                 toWhere[i] = count++;
//             }
//             newIndexVars.push_back(storedIndexVars[toWhere.at(i)]);
//         }
//         expr = new AccessNode(op->tensorVar, newIndexVars);
//     }
// };
}


