#pragma once

#include "TacoTensorBlock.h"

#include "LambdaCreationFunctions.h"
#include "TacoModuleMap.h"

#include "TAssignment.h"

using namespace pdb;

class TacoJoinAux: public Object {
public:
    ENABLE_DEEP_COPY

    TacoJoinAux() = default;

    TacoJoinAux(Handle<TAssignment> assignmentIn)
        : myAssignment(assignmentIn)
    {
        // TODO: if assignmentIn is nullptr here or anywhere else, very bad.
        myAssignment->diagnostic();
    }

    // handle of pointers is messy. Copying the handles breaks things,
    // and pointers are an easy way to store the handles by reference
    LambdaTree<bool>
    getKeySelection(std::vector<Handle<TacoTensorBlockMeta>*> meta) {
        // creating aliases
        Vector<int>& whichInputL = myAssignment->whichInputL;
        Vector<int>& whichInputR = myAssignment->whichInputR;
        Vector<int>& whichIndexL = myAssignment->whichIndexL;
        Vector<int>& whichIndexR = myAssignment->whichIndexR;

        auto get = [](Handle<TacoTensorBlockMeta>* in, int const& index)
        {
            return makeLambda(*in, [&index](
                Handle<TacoTensorBlockMeta>& in)
                {
                    return in->access(index);
                });
        };

        auto out =
            get(meta[whichInputL[0]], whichIndexL[0]) ==
            get(meta[whichInputR[0]], whichIndexR[0]);

        for(int i = 1; i != whichInputL.size(); ++i) {
            out = out && (
                get(meta[whichInputL[i]], whichIndexL[i]) ==
                get(meta[whichInputR[i]], whichIndexR[i])
            );
        }

        return out;
    }

    // TODO have either this or getKeySelection but not both
    LambdaTree<bool>
    getSelection(std::vector<Handle<TacoTensorBlock>*> blocks) {
        // creating aliases
        Vector<int>& whichInputL = myAssignment->whichInputL;
        Vector<int>& whichInputR = myAssignment->whichInputR;
        Vector<int>& whichIndexL = myAssignment->whichIndexL;
        Vector<int>& whichIndexR = myAssignment->whichIndexR;

        auto get = [](Handle<TacoTensorBlock>* in, int const& index)
        {
            return makeLambda(*in, [&index](
                Handle<TacoTensorBlock>& in)
                {
                    return in->getKey()->access(index);
                });
        };

        auto out =
            get(blocks[whichInputL[0]], whichIndexL[0]) ==
            get(blocks[whichInputR[0]], whichIndexR[0]);

        for(int i = 1; i != whichInputL.size(); ++i) {
            out = out && (
                get(blocks[whichInputL[i]], whichIndexL[i]) ==
                get(blocks[whichInputR[i]], whichIndexR[i])
            );
        }

        return out;
    }

    Handle<TacoTensorBlockMeta>
    getKeyProjection(std::vector<Handle<TacoTensorBlockMeta>*> meta) {
        // create aliases
        Vector<int>& whichOut      = this->myAssignment->whichOut;
        Vector<int>& whichOutIndex = this->myAssignment->whichOutIndex;

        std::vector<uint32_t> idxs;
        idxs.reserve(whichOut.size());
        for(int i = 0; i != whichOut.size(); ++i) {
            Handle<TacoTensorBlockMeta>& toUse = *meta[whichOut[i]];
            idxs.push_back(toUse->access(whichOutIndex[i]));
        }

        Handle<TacoTensorBlockMeta> out = makeObject<TacoTensorBlockMeta>(idxs);

        return out;
    }

    // TODO: should I use && refref in vector arguments for the methods of this class?

    Handle<TacoTensor>
    getValueProjection(std::vector<Handle<TacoTensor>> tacoTensors) {
        // get the dimensions of the output taco tensor
        std::vector<int> dimensions = getOutputDimensions(tacoTensors);

        // get the output type.
        taco::Format outputFormat = getOutputFormat(tacoTensors);

        // create the output TacoTensor
        // assume all input TacoTensors have the same datatype
        tacoTensors[0] = makeObject<TacoTensor>(
            tacoTensors[1]->getDatatype(),
            dimensions,
            outputFormat);

        // create the assignment statement
        std::vector<taco::TensorVar> tensorVars;
        for(int i = 0; i != tacoTensors.size(); ++i) {
            tensorVars.push_back(tacoTensors[i]->getTensorVar());
        }
        taco::Assignment assignment = myAssignment->getAssignment(tensorVars);

        // get the function to compute tacoTensors[0]
        TacoModuleMap m;
        void* function = m[assignment];

        // - run the computation, this will allocate
        //   memory in tacoTensors[0] as necessary.
        // - it should also work regardless of the
        //   sparsity structure of each of the tacoTensors
        // - it should also work for whaver the underlying
        //   computation contained in myAssignment is
        TacoTensor::callKernel(function, tacoTensors);

        // TODO: do we need to further change the type of tacoTensors[0] here?

        return tacoTensors[0];
    }

public:
    // the TAssignment object stores the computation AST that this
    // join runs for each set of matching blocks
    Handle<TAssignment> myAssignment;

private:
    std::vector<int> getOutputDimensions(
        std::vector<Handle<TacoTensor>>& tacoTensors)
    {
        int order = myAssignment->whichOut.size();
        std::vector<int> dimensions;
        dimensions.reserve(order);
        for(int mode = 0; mode != order; ++mode) {
            int whichT = this->myAssignment->whichOut[mode];
            int idx    = this->myAssignment->whichOutIndex[mode];
            dimensions.push_back(tacoTensors[whichT]->getDimension(idx));
        }
        return dimensions;
    }

    taco::Format getOutputFormat(
        std::vector<Handle<TacoTensor>>& tacoTensors)
    {
        int order = myAssignment->whichOut.size();
        if(myAssignment->requiresDenseOutput) {
            // If the rhs contains an operation that would set all zeros
            // to a non-zero value, then the output should be fully dense
            return taco::Format(
                std::vector<taco::ModeFormatPack>(order, taco::dense));
        }

        // The output for axis i should be sparse only if all
        // modes on the rhs are also sparse
        std::vector<taco::ModeFormatPack> formatPack;
        formatPack.reserve(order);
        // TODO: this code is too ugly
        for(int mode = 0; mode != order; ++mode) {
            Vector<int>& modeToTensor      = this->myAssignment->modeToTensor[mode];
            Vector<int>& modeToTensorIndex = this->myAssignment->modeToTensorIndex[mode];
            bool dense = false;
            if(modeToTensor.size() == 0) {
                // This means there are no index mode on the right side, example:
                //   A(i,j) = B(j)
                // In general, should this axis be sparse or not?
                // TODO just keep it sparse for now
                dense = false;
            }
            for(int i = 0; i != modeToTensor.size(); ++i) {
                int whichTensor      = modeToTensor[i];
                int whichTensorIndex = modeToTensorIndex[i];
                if(tacoTensors[whichTensor]->isModeDense(whichTensorIndex)) {
                    dense = true;
                    break;
                }
            }
            formatPack.emplace_back(dense ? taco::dense : taco::sparse);
        }
        return taco::Format(formatPack);
    }
};

