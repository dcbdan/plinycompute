#pragma once

#include <Object.h>
#include <PDBVector.h>
#include <StringIntPair.h>

#include <vector>
#include <utility>

class TraMeta : public pdb::Object {
public:
  TraMeta(): info() {}

  ENABLE_DEEP_COPY

  TraMeta(pdb::Vector<pdb::StringIntPair> info): info(info) {}

  TraMeta(std::vector<std::pair<std::string, int>> const& input)
    : info(input.size(), input.size()) {
      for(int i = 0; i != input.size(); ++i) {
        info[i] = pdb::StringIntPair(input[i].first, input[i].second);
      }
  }

  int access(pdb::Handle<pdb::String> which) {
    for(int i = 0; i != info.size(); ++i) {
      if(*(info[i].myString) == *which) {
        return info[i].myInt;
      }
    }
    // TODO: how should errors be handled?
    throw std::runtime_error("TraMeta: Could not find key!");
    return -1;
  }

  int access(pdb::String which) {
    for(int i = 0; i != info.size(); ++i) {
      if(*(info[i].myString) == which) {
        return info[i].myInt;
      }
    }
    // TODO: how should errors be handled?
    throw std::runtime_error("TraMeta: Could not find key!");
    return -1;
  }

  size_t size() const {
    return info.size();
  }

  bool operator==(const TraMeta &other) const {
    if(info.size() != other.info.size()) {
      return false;
    }

    // The order of info doesn't matter. So for each key
    // in this, make sure the key is in other.
    // And then make sure the index of each is the same
    auto keyIsInAndEqualToOther = [&](pdb::String& s, int& ind) {
      for(int i = 0; i != other.info.size(); ++i) {
        if(s == *(other.info[i].myString) && ind == other.info[i].myInt)
          return true;
      }
      return false;
    };

    for(int i = 0; i != info.size(); ++i) {
      if(!keyIsInAndEqualToOther(*(info[i].myString), info[i].myInt)) {
        return false;
      }
    }

    return true;
  };

  size_t hash() const {
    // TODO: what is a good hash?
    size_t mult = 10000;
    size_t ret = 0;
    for(size_t i = 0; i != info.size(); ++i) {
      ret += mult*info[i].myInt;
      ret += 3;
    }
    return ret;
  }

  void relabel(std::string kin, std::string knew) {
    for(int i = 0; i != info.size(); ++i) {
      if(*(info[i].myString) == kin) {
        *(info[i].myString) = pdb::String(knew);
        return;
      }
    }
    throw std::runtime_error("TraMeta::relabel this should not happen!");
  }

  void drop(std::string k) {
    pdb::Vector<pdb::StringIntPair> newInfo(info.size()-1, 0);

    bool found = false;

    for(int i = 0; i != info.size(); ++i) {
      if(*(info[i].myString) == k) {
        if(found) {
          throw std::runtime_error("TraMeta::drop already found!");
        }
        found = true;
      } else {
        newInfo.push_back(info[i]);
      }
    }
    if(!found) {
      throw std::runtime_error("TraMeta::drop did not find");
    }
    info = newInfo;
  }

  pdb::Vector<pdb::StringIntPair> info;
};
