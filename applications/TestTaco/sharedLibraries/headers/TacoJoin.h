#pragma once

#include "TacoTensorBlock.h"

#include "LambdaCreationFunctions.h"
#include "JoinComp.h"
#include "TacoModuleMap.h"

#include "TAssignment.h"

using namespace pdb;

class TacoJoin: public JoinComp<
    TacoJoin,
    TacoTensorBlock,
    TacoTensorBlock,
    TacoTensorBlock> {
public:
    ENABLE_DEEP_COPY

    TacoJoin() = default;

    TacoJoin(Handle<TAssignment> assignmentIn)
        : myAssignment(assignmentIn)
    {
        std::cout << "JOIN WITH TASSIGNMENT" << std::endl;
        // TODO: if assignmentIn is nullptr here or anywhere else, very bad.
    }

    Lambda<bool>
    getKeySelection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2)
    {
        std::cout << "GET KEY SELECTION" << std::endl;

        if(myAssignment->whichInputL.size() == 0) {
            return makeLambda(in1, in2, [](
                Handle<TacoTensorBlockMeta>&,
                Handle<TacoTensorBlockMeta>&)
                {
                    return true;
                });
        }

        // handle of pointers is messy. Copying the handles breaks things,
        // and pointers are an easy way to store the handles by reference
        std::vector<Handle<TacoTensorBlockMeta>*> meta = {nullptr, &in1, &in2};

        // creating aliases
        Vector<int>& whichInputL = myAssignment->whichInputL;
        Vector<int>& whichInputR = myAssignment->whichInputR;
        Vector<int>& whichIndexL = myAssignment->whichIndexL;
        Vector<int>& whichIndexR = myAssignment->whichIndexR;

        // debuggin...
        for(int i = 0; i != whichInputL.size(); ++i) {
            std::cout << whichInputL[i] << ", " << whichIndexL[i] << "  |  ";
            std::cout << whichInputR[i] << ", " << whichIndexR[i] << std::endl;
        }
        std::cout << "...\n";

        auto get = [](Handle<TacoTensorBlockMeta>* in, int const& index)
        {
            std::cout << "index in get is " << index << std::endl;
            return makeLambda(*in, [&index](
                Handle<TacoTensorBlockMeta>& in)
                {
                    return in->access(index);
                });
        };

        auto out =
            get(meta[whichInputL[0]], whichIndexL[0]) ==
            get(meta[whichInputR[0]], whichIndexR[0]);

        std::cout << "????????????????????????????????????????????????" << std::endl;

        for(int i = 1; i != whichInputL.size(); ++i) {
            std::cout << "/?/         " << i << std::endl;

            out = out && (
                get(meta[whichInputL[i]], whichIndexL[i]) ==
                get(meta[whichInputR[i]], whichIndexR[i])
            );
        }

        return out;
    }

    Lambda<Handle<TacoTensorBlockMeta>>
    getKeyProjection(
        Handle<TacoTensorBlockMeta> in1,
        Handle<TacoTensorBlockMeta> in2)
    {
        std::cout << "GET KEY PROJECTION" << std::endl;

        return makeLambda(in1, in2, [this](
            Handle<TacoTensorBlockMeta>& in1,
            Handle<TacoTensorBlockMeta>& in2)
        {
            std::cout << "GET KEY PROJECTION LAMBDA" << std::endl;

            Handle<TacoTensorBlockMeta> in0 = nullptr;
            std::vector<Handle<TacoTensorBlockMeta>> meta = {in0, in1, in2};

            // create aliases
            Vector<int>& whichOut      = this->myAssignment->whichOut;
            Vector<int>& whichOutIndex = this->myAssignment->whichOutIndex;

            std::vector<uint32_t> idxs(whichOut.size());
            for(int i = 0; i != whichOut.size(); ++i) {
                idxs.push_back(meta[whichOut[i]]->access(whichOutIndex[i]));
            }

            Handle<TacoTensorBlockMeta> out = makeObject<TacoTensorBlockMeta>(idxs);
            return out;
        });
    }

    Lambda<Handle<TacoTensor>>
    getValueProjection(
        Handle<TacoTensor> in1,
        Handle<TacoTensor> in2)
    {
        std::cout << "GET VALUE PROJECTION" << std::endl;

        return makeLambda(in1, in2, [this](
        Handle<TacoTensor>& in1,
        Handle<TacoTensor>& in2)
        {
            std::cout << "GET VALUE PROJECTION LAMBDA" << std::endl;

            // TODO make output type be the least dense viable output type?
            //      What should the output typpe be?

            std::vector<Handle<TacoTensor>> tacoTensors{nullptr, in1, in2};

            // get the dimensions of the output taco tensor
            std::vector<int> dimensions;
            int order = myAssignment->whichOut.size();
            dimensions.reserve(order);
            for(int mode = 0; mode != order; ++mode) {
                int whichT = this->myAssignment->whichOut[mode];
                int idx    = this->myAssignment->whichOutIndex[mode];
                dimensions.push_back(tacoTensors[whichT]->getDimension(idx));
            }

            // create the output TacoTensor. The output will be dense
            // throught out for now..
            // also assume all input TacoTensors have the same datatype
            Handle<TacoTensor> out = makeObject<TacoTensor>(
                in1->getDatatype(),
                dimensions,
                taco::Format(std::vector<taco::ModeFormatPack>(order, taco::dense)));

            // create the assignment statement
            std::vector<taco::TensorVar> tensorVars{out->getTensorVar(), in1->getTensorVar(), in2->getTensorVar()};
            taco::Assignment assignment = myAssignment->getAssignment(tensorVars);

            // get the function to compute out
            TacoModuleMap m;
            void* function = m[assignment];

            // - run the computation, this will allocate
            //   memory in out as necessary.
            // - it should also work regardless of the
            //   sparsity structure of out/in1/in2
            // - it should also work for whaver the underlying
            //   computation contained in myAssignment is
            TacoTensor::callKernel(function, {out, in1, in2});

            // TODO: do we need to further change the type of out here?

            return out;
        });
    }
private:
    // the TAssignment object stores the computation AST that this
    // join runs for each set of matching blocks
    Handle<TAssignment> myAssignment;
//private:
//    Lambda<Vector<uint32_t> > getFixed(
//        Handle<TacoTensorBlockMeta> in,
//        pdb::Vector<int>& fixed)
//    {
//        // fixed is caught by reference in the C++ lambda.
//        // this shouldn't cause any problems
//        return makeLambda(in1, [&fixed](Handle<TacoTensorBlockMeta>& in) {
//            size_t size = fixed.size();
//            pdb::Vector<uint32_t> out(size);
//            for(size_t idx = 0; idx != size; ++idx) {
//                out.push_back(in->access(fixed[idx]));
//            }
//            return out;
//        });
//    }
};
