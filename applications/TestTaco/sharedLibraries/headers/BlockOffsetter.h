#pragma once

#include <algorithm>

struct BlockOffsetter {
    BlockOffsetter(
        std::vector<uint32_t> dimension,
        std::vector<uint32_t> blockDimension,
        std::vector<uint32_t> block)
        : dimension(dimension),
          blockDimension(blockDimension),
          block(block)
    {}

    std::vector<int> getDimensionOfBlock() const {
        auto order = dimension.size();
        std::vector<int> out;
        out.reserve(order);
        for(int mode = 0; mode != order; ++mode) {
            auto& n = dimension[mode];
            auto& b = blockDimension[mode];
            auto& i = block[mode];
            out.push_back(n / b + (i < (n % b)));
        }

        return out;
    }

    uint32_t getNumBlocks() const {
        uint32_t out = 1;
        for(int i = 0; i != blockDimension.size(); ++i) {
            out *= blockDimension[i];
        }
        return out;
    }

    template <typename T, typename U>
    void getRange(std::vector<T>& beg, std::vector<U>& end) {
        auto order = dimension.size();
        beg.clear();
        end.clear();
        for(uint32_t mode = 0; mode != order; ++mode) {
            auto& n = dimension[mode];
            auto& b = blockDimension[mode];
            auto& i = block[mode];
            beg.push_back((n / b) * i     + std::min(i,   n % b));
            end.push_back((n / b) * (i+1) + std::min(i+1, n % b));
        }
    }

    template <typename T>
    bool inBlock(std::vector<T> const& coord) const {
        auto order = dimension.size();
        if(order != coord.size()) {
            return false; // this should really be an error
        }

        for(uint32_t mode = 0; mode != order; ++mode) {
            auto& n = dimension[mode];
            auto& b = blockDimension[mode];
            auto& i = block[mode];
            auto beg = (n / b) * i     + std::min(i,   n % b);
            auto end = (n / b) * (i+1) + std::min(i+1, n % b);

            if(coord[mode] < beg || coord[mode] >= end) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    std::vector<int> getCoordInBlock(std::vector<T> coord) const
    {
        auto order = dimension.size();
        std::vector<int> blockCoord;
        blockCoord.reserve(coord.size());
        for(uint32_t mode = 0; mode != order; ++mode) {
            auto& n = dimension[mode];
            auto& b = blockDimension[mode];
            auto& i = block[mode];
            auto beg = (n / b) * i+ std::min(i, n % b);
            blockCoord.push_back(coord[mode] - beg);
        }
        return blockCoord;
    }

private:
    std::vector<uint32_t> dimension;
    std::vector<uint32_t> blockDimension;
    std::vector<uint32_t> block;
};
