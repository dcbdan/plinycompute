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
#ifndef PDBCLIENT_H
#define PDBCLIENT_H

#include "PDBCatalogClient.h"
#include "PDBDispatcherClient.h"
#include "ServerFunctionality.h"

#include "Handle.h"
#include "PDBVector.h"
#include "HeapRequest.h"
#include "PDBStorageManagerClient.h"

/**
 * This class provides functionality so users can connect and access
 * different Server functionalities of PDB, such as:
 *
 *  Catalog services
 *  Dispatcher services
 *  Distributed Storage services
 *  Query services
 *
 */

namespace pdb {

    class PDBClient : public ServerFunctionality {

public:

      /**
       * Creates PDBClient
       * @param portIn - the port number of the PDB manager server
       * @param addressIn - the IP address of the PDB manager server
       */
      PDBClient(int portIn, std::string addressIn);

      PDBClient() = default;

      ~PDBClient() = default;

      void registerHandlers(PDBServer &forMe) override {};

      /**
       * Returns the error message generated by a function call
       * @return
       */
      string getErrorMessage();

      /**
       * Sends a request to the Catalog Server to shut down the server that we are connected to
       * returns true on success
       * @return - true if we succeeded false otherwise
       */
      bool shutDownServer();

      /**
       * Creates a database
       * @param databaseName - the name of the database we want to create
       * @return - true if we succeed
       */
      bool createDatabase(const std::string &databaseName);

      /**
       * Creates a set with a given type (using a template) for an existing
       * database, no page_size needed in arg.
       *
       * @tparam DataType - the type of the data the set stores
       * @param databaseName - the name of the database
       * @param setName - the name of the set we want to create
       * @return - true if we succeed
       */
      template <class DataType>
      bool createSet(const std::string &databaseName, const std::string &setName);

      /**
       * Sends a request to the Catalog Server to register a user-defined type defined in a shared library.
       * @param fileContainingSharedLib - the file that contains the library
       * @return
       */
      bool registerType(const std::string &fileContainingSharedLib);

      /**
       * Returns an iterator to the set
       * @tparam DataType - the type of the data of the set
       * @param dbName - the name of the database
       * @param setName - the name of the set
       * @return true if we succeed false otherwise
       */
      template <class DataType>
      PDBStorageIteratorPtr<DataType> getSetIterator(std::string dbName, std::string setName);

      /**
       * Lists all metadata registered in the catalog.
       */
      void listAllRegisteredMetadata();

      /**
       * Lists the Databases registered in the catalog.
       */
      void listRegisteredDatabases();

      /**
       * Lists the Sets for a given database registered in the catalog.
       * @param databaseName - the name of the database we want to list the sets for
       */
      void listRegisteredSetsForADatabase(const std::string &databaseName);

      /**
       * Lists the Nodes registered in the catalog.
       */
      void listNodesInCluster();

      /**
       * Lists user-defined types registered in the catalog.
       */
      void listUserDefinedTypes();

      /**
       * Send the data to be stored in a set
       * @param setAndDatabase - the database name and set name
       * @return
       */
      template <class DataType>
      bool sendData(const std::string &database, const std::string &set, Handle<Vector<Handle<DataType>>> dataToSend);

    private:
      std::shared_ptr<pdb::PDBCatalogClient> catalogClient;
      std::shared_ptr<pdb::PDBDispatcherClient> dispatcherClient;
      std::shared_ptr<PDBStorageManagerClient> storageManagerClient;

      std::function<bool(Handle<SimpleRequestResult>)>
      generateResponseHandler(std::string description, std::string &errMsg);

      // Port of the PlinyCompute manager node
      int port;

      // IP address of the PlinyCompute manager node
      std::string address;

      // Error Message (if an error occurred)
      std::string errorMsg;

      // Message returned by a PlinyCompute function
      std::string returnedMsg;

      // Client logger
      PDBLoggerPtr logger;
    };
}

#include "PDBClientTemplate.cc"
#include "PDBDispatcherClientTemplate.cc"

#endif
