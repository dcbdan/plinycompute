//
// Created by dimitrije on 3/2/19.
//

#ifndef PDB_PDBSOURCEPAGESETSPEC_H
#define PDB_PDBSOURCEPAGESETSPEC_H

namespace pdb {

enum PDBSourceType {
  MergeSource,
  SetScanSource,
  AggregationSource,
  ShuffledAggregatesSource,
  ShuffledJoinTuplesSource,
  JoinedShuffleSource,
  BroadcastJoinSource,
  BroadcastIntermediateJoinSource
};

// PRELOAD %PDBSourcePageSetSpec%

struct PDBSourcePageSetSpec : public Object {

public:

  ENABLE_DEEP_COPY

  /**
   * The type of the source
   */
  PDBSourceType sourceType;

  /**
   * Each page set is identified by a integer and a string. Generally set to (computationID, tupleSetIdentifier)
   * but relying on that is considered bad practice
   */
  std::pair<size_t, pdb::String> pageSetIdentifier;
};

}

#endif //PDB_PDBSOURCEPAGESETSPEC_H
