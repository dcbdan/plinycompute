#include <GreedyPlanner.h>
#include <thread>
#include "JoinAggPlanner.h"
#include "GeneticAggGroupPlanner.h"

pdb::JoinAggPlanner::JoinAggPlanner(const pdb::PDBAnonymousPageSetPtr &joinAggPageSet,
                                    uint32_t numNodes,
                                    uint32_t numThreads,
                                    const PDBPageHandle &pageToStore) : numNodes(numNodes),
                                                                        numThreads(numThreads) {
  // get the input page
  this->inputPage = joinAggPageSet->getNextPage(0);
  if(this->inputPage == nullptr) {
    throw runtime_error("There are no keys to do planning...");
  }
  this->inputPage->repin();

  // page to store
  this->pageToStore = pageToStore;

  // we have not executed anything
  this->num_finished = 0;

  // grab the copy of the aggGroups object
  auto *record = (Record<TIDIndexMap> *) inputPage->getBytes();
  aggGroups = record->getRootObject();
}

void pdb::JoinAggPlanner::doPlanning() {

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  // we need this for the planner
  std::vector<char> lhsRecordPositions;
  lhsRecordPositions.reserve(100 * numNodes);

  std::vector<char> rhsRecordPositions;
  rhsRecordPositions.reserve(100 * numNodes);

  std::vector<std::vector<int32_t>> aggregationGroups;
  aggregationGroups.resize(this->aggGroups->size());

  // figure the number of join groups berforehand
  auto numJoinGroups = 0;
  for (auto it = this->aggGroups->begin(); it != this->aggGroups->end(); ++it) { numJoinGroups += (*it).value.size(); }

  // resize the join groups the the right size
  // join groups belonging to the same aggregation group will be grouped together
  std::vector<PipJoinAggPlanResult::JoinedRecord> joinGroups;
  joinGroups.resize(numJoinGroups);

  // I use these to keep track of what
  int32_t currentJoinTID = 0;
  int32_t currentAggGroup = 0;
  for (auto it = this->aggGroups->begin(); it != this->aggGroups->end(); ++it) {

    /// 0. Round robing the aggregation groups

    // assign the
    TIDVector &joinedTIDs = (*it).value;
    auto &aggTID = (*it).key;

    // the join pairs
    for (size_t i = 0; i < joinedTIDs.size(); ++i) {

      // get the left tid
      auto leftTID = joinedTIDs[i].first.first;
      auto leftTIDNode = joinedTIDs[i].first.second;

      // get the right tid
      auto rightTID = joinedTIDs[i].second.first;
      auto rightTIDNode = joinedTIDs[i].second.second;

      // store the join group
      joinGroups[currentJoinTID] = { leftTID, rightTID, aggTID };

      // resize if necessary
      if (lhsRecordPositions.size() <= ((leftTID + 1) * numNodes)) {
        lhsRecordPositions.resize(((leftTID + 1) * numNodes));
      }

      // resize if necessary
      if (rhsRecordPositions.size() <= ((rightTID + 1) * numNodes)) {
        rhsRecordPositions.resize(((rightTID + 1) * numNodes));
      }

      // set the tids
      lhsRecordPositions[leftTID * numNodes + leftTIDNode] = true;
      rhsRecordPositions[rightTID * numNodes + rightTIDNode] = true;

      // set the tid to the group
      aggregationGroups[currentAggGroup].emplace_back(currentJoinTID);

      // go to the next join TID
      currentJoinTID++;
    }

    // we finished processing an aggregation group
    currentAggGroup++;
  }

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::cout << "Prep run : " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "[ns]" << '\n';

  // do the planning with just aggregation
  std::thread th_do_agg_first([&](){
    doAggFirstPlanning(lhsRecordPositions, rhsRecordPositions, aggregationGroups, joinGroups);
  });

  // do the planning with just join
  std::thread th_do_join_first([&](){
    doJoinFirstPlanning(lhsRecordPositions, rhsRecordPositions, aggregationGroups, joinGroups);
  });

  // do the planning with just join
  std::thread th_do_full([&](){
    doFullPlanning(lhsRecordPositions, rhsRecordPositions, aggregationGroups, joinGroups);
  });

  // wait for this to finish
  th_do_agg_first.join();
  th_do_join_first.join();
  th_do_full.join();

  std::cout << "Finished Planning\n";
}

void pdb::JoinAggPlanner::doAggFirstPlanning(const std::vector<char> &lhsRecordPositions,
                                             const std::vector<char> &rhsRecordPositions,
                                             const std::vector<std::vector<int32_t>> &aggregationGroups,
                                             const std::vector<PipJoinAggPlanResult::JoinedRecord> &joinGroups) {

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  pdb::GreedyPlanner::costs_t c{};
  c.agg_cost = 1;
  c.join_cost = 1;
  c.join_rec_size = 1;
  c.send_coef = 1;
  c.rhs_input_rec_size = 1;
  c.lhs_input_rec_size = 1;
  c.aggregation_rec_size = 1;

  // init the planner run the agg only planner
  pdb::GreedyPlanner planner(numNodes, c, lhsRecordPositions, rhsRecordPositions, aggregationGroups, joinGroups);

  // run for a number of iterations
  planner.run_agg_first_only();

  // get the result of the planning
  auto result = planner.get_agg_result();

  // update the planning costs
  planning_costs[(int32_t) AlgorithmID::AGG_FIRST_ONLY] = planner.getCost();

  // spin while all the other planners don't finish
  num_finished++;
  while (num_finished != 3) {}

  // the algorithm we selected
  auto choice = selectAlgorithm();

  // if the choice aggregation algorithm then do the rest of the planning
  if (choice == AlgorithmID::AGG_FIRST_ONLY) {

    // repin the page
    pageToStore->repin();
    UseTemporaryAllocationBlock blk{pageToStore->getBytes(), pageToStore->getSize()};

    // make the plan result object
    Handle<PipJoinAggPlanResult> planResult = pdb::makeObject<PipJoinAggPlanResult>(numNodes);

    // set the number of aggregation groups
    planResult->numAggGroups = this->aggGroups->size();

    // go through the map and do two things
    // assign aggregation groups to nodes
    for (auto it = this->aggGroups->begin(); it != this->aggGroups->end(); ++it) {

      /// 0. Round robing the aggregation groups

      // the aggregation tid
      auto &aggTID = (*it).key;

      // get the current node
      auto assignedNode = result[aggTID];

      // assign the aggregation group to the node
      (*planResult->aggToNode)[aggTID] = assignedNode;

      // increment the number of aggregation groups
      (*planResult->numAggGroupsPerNode)[assignedNode]++;

      // assign the
      TIDVector &joinedTIDs = (*it).value;

      // go through each joined key that makes up this and store what node we need to send it
      for (size_t i = 0; i < joinedTIDs.size(); ++i) {

        /// 1.0 Store the left side
        {
          // make sure we have it
          if ((*planResult->leftToNode).count(joinedTIDs[i].first.first) == 0) {
            (*planResult->leftToNode)[joinedTIDs[i].first.first] = Vector<bool>(numNodes, numNodes);
            (*planResult->leftToNode)[joinedTIDs[i].first.first].fill(false);
          }

          // grab the vector for the key tid
          (*planResult->leftToNode)[joinedTIDs[i].first.first][assignedNode] = true;
        }

        /// 1.1 Store the right side
        {
          // make sure we have it
          if ((*planResult->rightToNode).count(joinedTIDs[i].second.first) == 0) {
            (*planResult->rightToNode)[joinedTIDs[i].second.first] = Vector<bool>(numNodes, numNodes);
            (*planResult->rightToNode)[joinedTIDs[i].second.first].fill(false);
          }

          // grab the vector for the key tid
          (*planResult->rightToNode)[joinedTIDs[i].second.first][assignedNode] = true;
        }

        /// 1.2 Store the join group
        {
          (*planResult->joinGroupsPerNode)[assignedNode].push_back({joinedTIDs[i].first.first, joinedTIDs[i].second.first, aggTID});
        }
      }
    }

    // if did the agg only planning then the aggregation is logcal
    planResult->isLocalAggregation = true;

    // set the main record of the page
    getRecord(planResult);

    // store the choice
    selectedAlgorithm = AlgorithmID::AGG_FIRST_ONLY;
  }

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  agg_first_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
}

void pdb::JoinAggPlanner::doJoinFirstPlanning(const std::vector<char> &lhsRecordPositions,
                                              const std::vector<char> &rhsRecordPositions,
                                              const std::vector<std::vector<int32_t>> &aggregationGroups,
                                              const std::vector<PipJoinAggPlanResult::JoinedRecord> &joinGroups) {

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  pdb::GreedyPlanner::costs_t c{};
  c.agg_cost = 1;
  c.join_cost = 1;
  c.join_rec_size = 1;
  c.send_coef = 1;
  c.rhs_input_rec_size = 1;
  c.lhs_input_rec_size = 1;
  c.aggregation_rec_size = 1;

  // init the planner run the agg only planner
  pdb::GreedyPlanner planner(numNodes, c, lhsRecordPositions, rhsRecordPositions, aggregationGroups, joinGroups);

  // run for a number of iterations
  planner.run_join_first_only();

  // get the result of the planning
  auto result = planner.get_result();

  // update the planning costs
  planning_costs[(int32_t) AlgorithmID::JOIN_FIRST_ONLY] = planner.getCost();

  // spin while all the other planners don't finish
  num_finished++;
  while (num_finished != 3) {}

  // the algorithm we selected
  auto choice = selectAlgorithm();

  // did we chose the join first only strategy if so init the plan
  if (choice == AlgorithmID::JOIN_FIRST_ONLY) {

    // repin the page
    pageToStore->repin();
    UseTemporaryAllocationBlock blk{pageToStore->getBytes(), pageToStore->getSize()};

    // make the plan result object
    Handle<PipJoinAggPlanResult> planResult = pdb::makeObject<PipJoinAggPlanResult>(numNodes);

    // set the number of aggregation groups
    planResult->numAggGroups = this->aggGroups->size();

    // go through the map and do two things
    // assign aggregation groups to nodes
    for (auto it = this->aggGroups->begin(); it != this->aggGroups->end(); ++it) {

      /// 0. Round robing the aggregation groups

      // the aggregation tid
      auto &aggTID = (*it).key;

      // get the current node
      auto assignedNode = result.agg_group_assignments[aggTID];

      // assign the aggregation group to the node
      (*planResult->aggToNode)[aggTID] = assignedNode;
    }

    // keeps track of what the last aggregation group was
    vector<int32_t> lastAggGroup(numNodes, -1);

    // go through each join group
    for(auto jg = 0; jg < joinGroups.size(); ++jg) {
      int num_assigned = 0;
      for(auto node = 0; node < numNodes; ++node) {

        if (result.join_groups_to_node[jg * numNodes + node]) {

          /// 1.0 Store the left side
          {
            // make sure we have it
            if ((*planResult->leftToNode).count(joinGroups[jg].lhsTID) == 0) {
              (*planResult->leftToNode)[joinGroups[jg].lhsTID] = Vector<bool>(numNodes, numNodes);
              (*planResult->leftToNode)[joinGroups[jg].lhsTID].fill(false);
            }

            // grab the vector for the key tid
            (*planResult->leftToNode)[joinGroups[jg].lhsTID][node] = true;
          }

          /// 1.1 Store the right side
          {
            // make sure we have it
            if ((*planResult->rightToNode).count(joinGroups[jg].rhsTID) == 0) {
              (*planResult->rightToNode)[joinGroups[jg].rhsTID] = Vector<bool>(numNodes, numNodes);
              (*planResult->rightToNode)[joinGroups[jg].rhsTID].fill(false);
            }

            // grab the vector for the key tid
            (*planResult->rightToNode)[joinGroups[jg].rhsTID][node] = true;
          }

          /// 1.2 Store the join group
          (*planResult->joinGroupsPerNode)[node].push_back({joinGroups[jg].lhsTID, joinGroups[jg].rhsTID, joinGroups[jg].aggTID});
          num_assigned++;

          /// 1.3 Update the num of aggregation groups per node if this is a new aggregation group
          (*planResult->numAggGroupsPerNode)[node] += joinGroups[jg].aggTID != lastAggGroup[node];
          lastAggGroup[node] = joinGroups[jg].aggTID;
        }
      }

      if(num_assigned != 1) {
        std::cout << "Ninja\n";
      }
    }

    // set the main record of the page
    getRecord(planResult);

    // store the choice
    selectedAlgorithm = AlgorithmID::JOIN_FIRST_ONLY;
  }

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  join_first_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
}

void pdb::JoinAggPlanner::doFullPlanning(const std::vector<char> &lhsRecordPositions,
                                         const std::vector<char> &rhsRecordPositions,
                                         const std::vector<std::vector<int32_t>> &aggregationGroups,
                                         const std::vector<PipJoinAggPlanResult::JoinedRecord> &joinGroups) {

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  pdb::GreedyPlanner::costs_t c{};
  c.agg_cost = 1;
  c.join_cost = 1;
  c.join_rec_size = 1;
  c.send_coef = 1;
  c.rhs_input_rec_size = 1;
  c.lhs_input_rec_size = 1;
  c.aggregation_rec_size = 1;

  // init the planner run the agg only planner
  pdb::GreedyPlanner planner(numNodes, c, lhsRecordPositions, rhsRecordPositions, aggregationGroups, joinGroups);

  // run for a number of iterations
  planner.run();

  // get the result of the planning
  auto result = planner.get_result();

  // update the planning costs
  planning_costs[(int32_t) AlgorithmID::FULL] = planner.getCost();

  // spin while all the other planners don't finish
  num_finished++;
  while (num_finished != 3) {}

  // the algorithm we selected
  auto choice = selectAlgorithm();

  //
  if (choice == AlgorithmID::FULL) {

    // repin the page
    pageToStore->repin();
    UseTemporaryAllocationBlock blk{pageToStore->getBytes(), pageToStore->getSize()};

    // make the plan result object
    Handle<PipJoinAggPlanResult> planResult = pdb::makeObject<PipJoinAggPlanResult>(numNodes);

    // set the number of aggregation groups
    planResult->numAggGroups = this->aggGroups->size();

    // go through the map and do two things
    // assign aggregation groups to nodes
    for (auto it = this->aggGroups->begin(); it != this->aggGroups->end(); ++it) {

      /// 0. Round robing the aggregation groups

      // the aggregation tid
      auto &aggTID = (*it).key;

      // get the current node
      auto assignedNode = result.agg_group_assignments[aggTID];

      // assign the aggregation group to the node
      (*planResult->aggToNode)[aggTID] = assignedNode;
    }

    // keeps track of what the last aggregation group was
    vector<int32_t> lastAggGroup(numNodes, -1);

    // go through each join group
    for(auto jg = 0; jg < joinGroups.size(); ++jg) {

      for(auto node = 0; node < numNodes; ++node) {
        if (result.join_groups_to_node[jg * numNodes + node]) {

          /// 1.0 Store the left side
          {
            // make sure we have it
            if ((*planResult->leftToNode).count(joinGroups[jg].lhsTID) == 0) {
              (*planResult->leftToNode)[joinGroups[jg].lhsTID] = Vector<bool>(numNodes, numNodes);
              (*planResult->leftToNode)[joinGroups[jg].lhsTID].fill(false);
            }

            // grab the vector for the key tid
            (*planResult->leftToNode)[joinGroups[jg].lhsTID][node] = true;
          }

          /// 1.1 Store the right side
          {
            // make sure we have it
            if ((*planResult->rightToNode).count(joinGroups[jg].rhsTID) == 0) {
              (*planResult->rightToNode)[joinGroups[jg].rhsTID] = Vector<bool>(numNodes, numNodes);
              (*planResult->rightToNode)[joinGroups[jg].rhsTID].fill(false);
            }

            // grab the vector for the key tid
            (*planResult->rightToNode)[joinGroups[jg].rhsTID][node] = true;
          }

          /// 1.2 Store the join group
          (*planResult->joinGroupsPerNode)[node].push_back({joinGroups[jg].lhsTID, joinGroups[jg].rhsTID, joinGroups[jg].aggTID});

          /// 1.3 Update the num of aggregation groups per node if this is a new aggregation group
          (*planResult->numAggGroupsPerNode)[node] += joinGroups[jg].aggTID != lastAggGroup[node];
          lastAggGroup[node] = joinGroups[jg].aggTID;
        }
      }
    }

    // set the main record of the page
    getRecord(planResult);

    // store the choice
    selectedAlgorithm = AlgorithmID::FULL;
  }

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  full_first_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
}

bool pdb::JoinAggPlanner::isLocalAggregation() {
  return selectedAlgorithm == AlgorithmID::AGG_FIRST_ONLY;
}

pdb::JoinAggPlanner::AlgorithmID pdb::JoinAggPlanner::selectAlgorithm() {

  // we prefer the agg first planning since it does not have a later shuffling phase
//  if (planning_costs[(int32_t) AlgorithmID::AGG_FIRST_ONLY] <= planning_costs[(int32_t) AlgorithmID::JOIN_FIRST_ONLY] &&
//      planning_costs[(int32_t) AlgorithmID::AGG_FIRST_ONLY] <= planning_costs[(int32_t) AlgorithmID::FULL]) {
//    return AlgorithmID::AGG_FIRST_ONLY;
//  } else if (planning_costs[(int32_t) AlgorithmID::JOIN_FIRST_ONLY] < planning_costs[(int32_t) AlgorithmID::FULL]) {
    return AlgorithmID::JOIN_FIRST_ONLY;
//  } else {
//    return AlgorithmID::FULL;
//  }
}

void pdb::JoinAggPlanner::print(const Handle<PipJoinAggPlanResult> &planResult) {

  for (auto it = planResult->leftToNode->begin(); it != planResult->leftToNode->end(); ++it) {

    std::cout << "Left TID " << (*it).key << " goes to:\n";
    Vector<bool> &nodes = (*it).value;
    for (int i = 0; i < nodes.size(); ++i) {
      if (nodes[i]) {
        std::cout << "\tNode " << i << "\n";
      }
    }
  }

  std::cout << "\n\n";

  for (auto it = planResult->rightToNode->begin(); it != planResult->rightToNode->end(); ++it) {

    std::cout << "Right TID " << (*it).key << " goes to:\n";
    Vector<bool> &nodes = (*it).value;
    for (int i = 0; i < nodes.size(); ++i) {
      if (nodes[i]) {
        std::cout << "\tNode " << i << "\n";
      }
    }
  }

  std::cout << "\n\n";

  for (auto it = planResult->aggToNode->begin(); it != planResult->aggToNode->end(); ++it) {
    std::cout << "Aggregation Group" << (*it).key << " goes to " << (*it).value << "\n";
  }
  std::cout << "\n\n";
}
