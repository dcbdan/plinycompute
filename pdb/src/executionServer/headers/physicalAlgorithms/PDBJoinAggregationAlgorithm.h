#pragma once

#include <gtest/gtest_prod.h>
#include <PipelineInterface.h>
#include <processors/PreaggregationPageProcessor.h>
#include "PDBPhysicalAlgorithm.h"
#include "PDBPageSelfReceiver.h"
#include "Computation.h"
#include "PDBPageNetworkSender.h"
#include "PDBAnonymousPageSet.h"
#include "JoinAggPlanner.h"
#include "PDBLabeledPageSet.h"
#include "JoinAggSideSender.h"
#include "JoinMapCreator.h"

namespace pdb {

// PRELOAD %PDBJoinAggregationAlgorithm%

/**
 * Basically executes a pipeline that looks like this :
 *
 *        agg
 *         |
 *        join
 *       /    \
 *     lhs   rhs
 *
 * This algorithm should only be use for cases where there is a few records that are very large
 */
class PDBJoinAggregationAlgorithm : public PDBPhysicalAlgorithm {
 public:

  ENABLE_DEEP_COPY

  PDBJoinAggregationAlgorithm() = default;

  PDBJoinAggregationAlgorithm(const std::vector<PDBPrimarySource> &leftSource,
                              const std::vector<PDBPrimarySource> &rightSource,
                              const pdb::Handle<PDBSinkPageSetSpec> &sink,
                              const pdb::Handle<PDBSinkPageSetSpec> &leftKeySink,
                              const pdb::Handle<PDBSinkPageSetSpec> &rightKeySink,
                              const pdb::Handle<PDBSinkPageSetSpec> &joinAggKeySink,
                              const pdb::Handle<PDBSinkPageSetSpec> &intermediateSink,
                              const pdb::Handle<PDBSourcePageSetSpec> &leftKeySource,
                              const pdb::Handle<PDBSourcePageSetSpec> &rightKeySource,
                              const pdb::Handle<PDBSourcePageSetSpec> &leftJoinSource,
                              const pdb::Handle<PDBSourcePageSetSpec> &rightJoinSource,
                              const pdb::Handle<PDBSourcePageSetSpec> &planSource,
                              const AtomicComputationPtr& leftInputTupleSet,
                              const AtomicComputationPtr& rightInputTupleSet,
                              const AtomicComputationPtr& joinTupleSet,
                              const AtomicComputationPtr& aggregationKey,
                              pdb::Handle<PDBSinkPageSetSpec> &hashedLHSKey,
                              pdb::Handle<PDBSinkPageSetSpec> &hashedRHSKey,
                              pdb::Handle<PDBSinkPageSetSpec> &aggregationTID,
                              const std::vector<pdb::Handle<PDBSourcePageSetSpec>> &secondarySources,
                              const pdb::Handle<pdb::Vector<PDBSetObject>> &setsToMaterialize);

  ~PDBJoinAggregationAlgorithm() override = default;

  /**
   * //TODO
   */
  bool setup(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage, Handle<pdb::ExJob> &job, const std::string &error) override;

  bool setupLead(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage, Handle<pdb::ExJob> &job, const std::string &error);

  bool setupFollower(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage, Handle<pdb::ExJob> &job, const std::string &error);

  /**
   * //TODO
   */
  bool run(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage, Handle<pdb::ExJob> &job) override;

  bool runLead(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage, Handle<pdb::ExJob> &job);

  bool runFollower(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage, Handle<pdb::ExJob> &job);

  /**
   *
   */
  void cleanup() override;

  /**
   * Returns StraightPipe as the type
   * @return the type
   */
  PDBPhysicalAlgorithmType getAlgorithmType() override;

  /**
   * The output container type of the straight pipeline is always a vector, meaning the root object is always a pdb::Vector
   * @return PDB_CATALOG_SET_VECTOR_CONTAINER
   */
  PDBCatalogSetContainerType getOutputContainerType() override;

  static pdb::SourceSetArgPtr getKeySourceSetArg(std::shared_ptr<pdb::PDBCatalogClient> &catalogClient,
                                                 pdb::Vector<PDBSourceSpec> &sources,
                                                 size_t idx);

  pdb::SourceSetArgPtr getRightSourceSetArg(std::shared_ptr<pdb::PDBCatalogClient> &catalogClient, size_t idx);

  static PDBAbstractPageSetPtr getKeySourcePageSet(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage,
                                                   size_t idx,
                                                   pdb::Vector<PDBSourceSpec> &srcs);

  static PDBAbstractPageSetPtr getFetchingPageSet(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage,
                                                  size_t idx,
                                                  pdb::Vector<PDBSourceSpec> &srcs,
                                                  const std::string &ip,
                                                  int32_t port);

  PDBAbstractPageSetPtr getRightSourcePageSet(std::shared_ptr<pdb::PDBStorageManagerBackend> &storage, size_t idx);


 private:

  /**
   *
   */
  bool setupSenders(Handle<pdb::ExJob> &job,
                    pdb::Handle<PDBSourcePageSetSpec> &recvPageSet,
                    std::shared_ptr<pdb::PDBStorageManagerBackend> &storage,
                    std::shared_ptr<std::vector<PDBPageQueuePtr>> &pageQueues,
                    std::shared_ptr<std::vector<PDBPageNetworkSenderPtr>> &senders,
                    PDBPageSelfReceiverPtr *selfReceiver);

  /**
   * The lhs input set to the join aggregation pipeline
   */
  pdb::String leftInputTupleSet;

  /**
   * The rhs input set to the join aggregation pipeline
   */
  pdb::String rightInputTupleSet;

  /**
   * The join tuple set
   */
  pdb::String joinTupleSet;

  /**
   * The sources of the right side of the merged pipeline
   */
  pdb::Vector<PDBSourceSpec> rightSources;

  /**
   * this page set is going to have the intermediate results of the LHS, the it is going to contain the JoinMap<hash, LHSKey>
   */
  pdb::Handle<PDBSinkPageSetSpec> hashedLHSKey;

  /**
   * this page set is going to have the intermediate results of the RHS, the it is going to contain the JoinMap<hash, RHSKey>
   */
  pdb::Handle<PDBSinkPageSetSpec> hashedRHSKey;

  /**
   * this page set is going to have the intermediate results of the Aggregation Keys, the it is going to contain the JoinMap<AGG_TID, Vector<pair<LHS_TID, RHS_TID>>
   * there are also going to be two anonymous pages with Map<LHSKey, LHS_TID> and Map<RHSKey, RHS_Key>.
   */
  pdb::Handle<PDBSinkPageSetSpec> aggregationTID;

  /**
   *
   */
  pdb::Handle<PDBSourcePageSetSpec> leftJoinSource;

  /**
   *
   */
  pdb::Handle<PDBSourcePageSetSpec> rightJoinSource;

  /**
   * The labled left page set of keys
   */
  pdb::PDBLabeledPageSetPtr labeledLeftPageSet = nullptr;

  /**
   * The labled right page set of keys
   */
  pdb::PDBLabeledPageSetPtr labeledRightPageSet = nullptr;

  /**
   *
   */
  pdb::PDBAnonymousPageSetPtr joinAggPageSet = nullptr;

  pdb::PDBAnonymousPageSetPtr leftShuffledPageSet = nullptr;

  pdb::PDBAnonymousPageSetPtr rightShuffledPageSet = nullptr;

  pdb::PDBAnonymousPageSetPtr intermediatePageSet = nullptr;

  pdb::PDBFeedingPageSetPtr preaggPageSet = nullptr;


  /**
   *
   */
  pdb::PDBFeedingPageSetPtr leftKeyToNodePageSet = nullptr;

  /**
   *
   */
  pdb::PDBFeedingPageSetPtr rightKeyToNodePageSet= nullptr;

  /**
   *
   */
  pdb::PDBFeedingPageSetPtr planPageSet = nullptr;

  /**
   *
   */
  PDBPageHandle leftKeyPage = nullptr;

  /**
   *
   */
  PDBPageHandle rightKeyPage = nullptr;

  /**
   *
   */
  PDBPageHandle planPage;

  /**
   *
   */
  std::shared_ptr<std::vector<PDBPageQueuePtr>> leftKeyPageQueues = nullptr;

  /**
   *
   */
  std::shared_ptr<std::vector<PDBPageQueuePtr>> rightKeyPageQueues = nullptr;

  /**
   *
   */
  std::shared_ptr<std::vector<PDBPageQueuePtr>> planPageQueues = nullptr;

  /**
   *
   */
  std::shared_ptr<std::vector<PDBPageNetworkSenderPtr>> leftKeySenders;

  /**
   *
   */
  std::shared_ptr<std::vector<PDBPageNetworkSenderPtr>> rightKeySenders;

  /**
   * This sends the plan
   */
  std::shared_ptr<std::vector<PDBPageNetworkSenderPtr>> planSenders;

  /**
   * Connections to nodes (including self connection to this one) for the left side of the join
   */
  std::shared_ptr<std::vector<PDBCommunicatorPtr>> leftJoinSideCommunicatorsOut = nullptr;

  /**
   * Connections to nodes (including self connection to this one) for the right side of the join
   */
  std::shared_ptr<std::vector<PDBCommunicatorPtr>> rightJoinSideCommunicatorsOut = nullptr;

  /**
   * Connections to nodes (including self connection to this one) for the left side of the join
   */
  std::shared_ptr<std::vector<PDBCommunicatorPtr>> leftJoinSideCommunicatorsIn = nullptr;

  /**
   * Connections to nodes (including self connection to this one) for the right side of the join
   */
  std::shared_ptr<std::vector<PDBCommunicatorPtr>> rightJoinSideCommunicatorsIn = nullptr;

  /**
   * These send records to the nodes for the left side
   */
  std::shared_ptr<std::vector<JoinAggSideSenderPtr>> leftJoinSideSenders = nullptr;

  /**
   * These send records to the nodes for the right side
   */
  std::shared_ptr<std::vector<JoinAggSideSenderPtr>> rightJoinSideSenders = nullptr;

  /**
   * This takes in the records from the side of the join and makes them into a tuple set
   */
  std::shared_ptr<std::vector<JoinMapCreatorPtr>> joinMapCreators = nullptr;

  /**
   * The join key side pipelines
   */
  std::shared_ptr<std::vector<PipelinePtr>> joinKeyPipelines = nullptr;

  /**
   * The preaggregation pipelines
   */
  std::shared_ptr<std::vector<PipelinePtr>> preaggregationPipelines = nullptr;

  /**
   * The aggregation pipelines
   */
  std::shared_ptr<std::vector<PipelinePtr>> aggregationPipelines = nullptr;

  /**
   *
   */
  std::shared_ptr<std::vector<PDBPageQueuePtr>> pageQueues = nullptr;


  /**
   * The left and right join side task
   */
  static const int32_t LEFT_JOIN_SIDE_TASK;
  static const int32_t RIGHT_JOIN_SIDE_TASK;

  /**
   * The join aggregation pipeline
   */
  PipelinePtr joinKeyAggPipeline = nullptr;

  /**
   * This runs the left and right side of the join
   */
  std::shared_ptr<std::vector<PipelinePtr>> joinPipelines = nullptr;

  FRIEND_TEST(TestPhysicalOptimizer, TestKeyedMatrixMultipply);
};

}