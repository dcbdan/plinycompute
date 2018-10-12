/*****************************************************************************
 *                                                                           *
 *  Copyright 2018 Rice University                                           *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *      http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 *****************************************************************************/

#include <boost/program_options.hpp>
#include <iostream>
#include <PDBServer.h>
#include <GenericWork.h>
#include <NodeConfig.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <ClusterManager.h>
#include <CatalogServer.h>

namespace po = boost::program_options;
namespace fs = boost::filesystem;


int main(int argc, char *argv[]) {

  // create the program options
  po::options_description desc{"Options"};

  // the configuration for this node
  auto config = make_shared<pdb::NodeConfig>();

  // specify the options
  desc.add_options()("help,h", "Help screen");
  desc.add_options()("isManager,m", po::bool_switch(&config->isManager), "Start manager");
  desc.add_options()("address,i", po::value<std::string>(&config->address)->default_value("localhost"), "IP of the node");
  desc.add_options()("port,p", po::value<int32_t>(&config->port)->default_value(8108), "Port of the node");
  desc.add_options()("managerAddress,d", po::value<std::string>(&config->managerAddress)->default_value("localhost"), "IP of the manager");
  desc.add_options()("managerPort,o", po::value<int32_t>(&config->managerPort)->default_value(8108), "Port of the manager");
  desc.add_options()("sharedMemSize,s", po::value<size_t>(&config->sharedMemSize)->default_value(2048), "The size of the shared memory (MB)");
  desc.add_options()("numThreads,t", po::value<int32_t>(&config->numThreads)->default_value(1), "The number of threads we want to use");
  desc.add_options()("rootDirectory,r", po::value<std::string>(&config->rootDirectory)->default_value("./pdbRoot"), "The root directory we want to use.");

  // grab the options
  po::variables_map vm;
  store(parse_command_line(argc, argv, desc), vm);
  notify(vm);

  // did somebody ask for help?
  if (vm.count("help")) {
    std::cout << desc << '\n';
    return 0;
  }

  // create the root directory
  fs::path rootPath(config->rootDirectory);
  if(!fs::exists(rootPath) && !fs::create_directories(rootPath)) {
    std::cout << "Failed to create the root directory!\n";
  }

  // init other parameters
  config->ipcFile = fs::path(config->rootDirectory).append("/ipcFile").string();
  config->catalogFile = fs::path(config->rootDirectory).append("/catalog").string();
  config->maxConnections = 100;

  // fork this to split into a frontend and backend
  pid_t pid = fork();

  // check whether we are the frontend or the backend
  if(pid == 0) {

    // do backend setup
    pdb::PDBLoggerPtr logger = make_shared<pdb::PDBLogger>("manager.log");
    pdb::PDBServer backEnd(pdb::PDBServer::NodeType::BACKEND, config, logger);

    // start the backend
    backEnd.startServer(nullptr);
  }
  else {

    // I'm the frontend server
    pdb::PDBLoggerPtr logger = make_shared<pdb::PDBLogger>("manager.log");
    pdb::PDBServer frontEnd(pdb::PDBServer::NodeType::FRONTEND, config, logger);

    // add the functionaries
    frontEnd.addFunctionality<pdb::ClusterManager>();
    frontEnd.addFunctionality<pdb::CatalogServer>();
    frontEnd.addFunctionality<pdb::CatalogClient>(config->port, config->address, logger);

    frontEnd.startServer(make_shared<pdb::GenericWork>([&](PDBBuzzerPtr callerBuzzer) {

      // sync me with the cluster
      std::string error;
      frontEnd.getFunctionality<pdb::ClusterManager>().syncCluster(error);

      // log that the server has started
      std::cout << "Distributed storage manager server started!\n";

      // buzz that we are done
      callerBuzzer->buzz(PDBAlarm::WorkAllDone);
    }));
  }

  return 0;
}
