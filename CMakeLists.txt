cmake_minimum_required(VERSION 3.5.1)

# set clang as the default compiler
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(pdb)

# set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/libraries")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/libraries")

# set the standard
set(CMAKE_CXX_STANDARD 14)

# check the operating system and add the appropriate flags
if (APPLE)
    SET(CMAKE_CXX_FLAGS "-Wall -g")
elseif (UNIX AND NOT APPLE)
    SET(CMAKE_CXX_FLAGS "-g -lsnappy -fPIC -Wno-deprecated-declarations")
endif ()

# set the definitions for the compiler
ADD_DEFINITIONS(-DINITIALIZE_ALLOCATOR_BLOCK)
ADD_DEFINITIONS(-DENABLE_SHALLOW_COPY)
ADD_DEFINITIONS(-DDEFAULT_BATCH_SIZE=100)
ADD_DEFINITIONS(-DREMOVE_SET_WITH_EVICTION)
ADD_DEFINITIONS(-DAUTO_TUNING)
ADD_DEFINITIONS(-DENABLE_LARGE_GRAPH)
ADD_DEFINITIONS(-DJOIN_HASH_TABLE_SIZE_RATIO=1.5)
ADD_DEFINITIONS(-DEVICT_STOP_THRESHOLD=0.95)
ADD_DEFINITIONS(-DEVICT_STOP_THRESHOLD=0.95)
ADD_DEFINITIONS(-DENABLE_COMPRESSION)

# set the directories with the common header files
include_directories("${PROJECT_SOURCE_DIR}/src/memory/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tcapParser/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/queryIrProcessor/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/utilities/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/optimizerBenchmark/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/pdbServer/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/queryPlanning/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/qunit/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tcapIntermediaryRep/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/dispatcher/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/logicalPlanTests/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/bufferMgr/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/lambdas/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tcapBuilderTests/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tcapParserTests/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tpchBench/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/work/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/distributionManager/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/communication/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/logicalPlan/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/queries/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/queriesTests/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tcapLexerTests/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/serverFunctionalities/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/queryIntermediaryRep/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/catalog/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/builtInPDBObjects/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tcapBuilder/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/conf/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tcapLexer/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/linearAlgebraDSL/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/tcapIntermediaryRepTests/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/sqlite/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/storage/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/queryIntermediaryRepTests/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/objectModel/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/dummyFolder/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/sharedLibraries/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/queryExecution/headers")
include_directories("${PROJECT_SOURCE_DIR}/src/boost")

# specify the source files
file(GLOB SERVER_FUNCTIONALITIES_SOURCE "${PROJECT_SOURCE_DIR}/src/serverFunctionalities/source/*.cc")
file(GLOB COMMUNICATION_SOURCE "${PROJECT_SOURCE_DIR}/src/communication/source/*.cc")
file(GLOB CATALOG_SOURCE "${PROJECT_SOURCE_DIR}/src/catalog/source/*.cc")
file(GLOB DISPATCHER_SOURCE "${PROJECT_SOURCE_DIR}/src/dispatcher/source/*.cc")
file(GLOB PDB_SERVER_SOURCE "${PROJECT_SOURCE_DIR}/src/pdbServer/source/*.cc")
file(GLOB OBJECT_MODEL_SOURCE "${PROJECT_SOURCE_DIR}/src/objectModel/source/*.cc")
file(GLOB QUERY_EXECUTION_SOURCE "${PROJECT_SOURCE_DIR}/src/queryExecution/source/*.cc")
file(GLOB QUERY_PANNING_SOURCE "${PROJECT_SOURCE_DIR}/src/queryPlanning/source/*.cc")
file(GLOB QUERY_INTERMEDIARY_REP_SOURCE "${PROJECT_SOURCE_DIR}/src/queryIntermediaryRep/source/*.cc")
file(GLOB WORK_SOURCE "${PROJECT_SOURCE_DIR}/src/work/source/*.cc")
file(GLOB MEMORY_SOURCE "${PROJECT_SOURCE_DIR}/src/memory/source/*.cc")
file(GLOB STORAGE_SOURCE "${PROJECT_SOURCE_DIR}/src/storage/source/*.cc")
file(GLOB DISTRIBUTION_MANAGER_SOURCE "${PROJECT_SOURCE_DIR}/src/distributionManager/source/*.cc")
file(GLOB LAMBDAS_SOURCE "${PROJECT_SOURCE_DIR}/src/lambdas/source/*.cc")
file(GLOB LOGICAL_PLAN_SOURCE "${PROJECT_SOURCE_DIR}/src/logicalPlan/source/*.cc")
file(GLOB LINEAR_ALGEBRA_DSL_SOURCE "${PROJECT_SOURCE_DIR}/src/linearAlgebraDSL/source/*.cc")
file(GLOB BOOST_FILESYSTEM_SOURCE "${PROJECT_SOURCE_DIR}/src/boost/libs/filesystem/src/*.cpp")
file(GLOB PROGRAM_OPTIONS_SOURCE "${PROJECT_SOURCE_DIR}/src/boost/libs/program_options/src/*.cpp")
file(GLOB SYSTEM_SOURCE "${PROJECT_SOURCE_DIR}/src/boost/libs/system/src/*.cpp")
set(SOURCE_FILES_SQL_LITE ${PROJECT_SOURCE_DIR}/src/sqlite/source/sqlite3.c)

# specify the hearder files
file(GLOB PDB_HEADERS "${PROJECT_SOURCE_DIR}/src/*/headers/*.h")
add_library(HEADER_ONLY_TARGET STATIC "${PDB_HEADERS}")
set_target_properties(HEADER_ONLY_TARGET PROPERTIES LINKER_LANGUAGE CXX)

# generate lexers
FIND_PACKAGE(FLEX REQUIRED)

# set the output paths
SET(LogicalPlanFlexOutput ${CMAKE_SOURCE_DIR}/src/logicalPlan/source/Lexer.c)
SET(LinearAlgebraDSLFlexOutput ${CMAKE_SOURCE_DIR}/src/linearAlgebraDSL/source/LALexer.c)

# setup the commands
IF (FLEX_FOUND)
    # logical plan lexer commands
    ADD_CUSTOM_COMMAND(
            OUTPUT ${LogicalPlanFlexOutput}
            COMMAND ${FLEX_EXECUTABLE}
            --outfile=${LogicalPlanFlexOutput}
            ${CMAKE_SOURCE_DIR}/src/logicalPlan/source/Lexer.l
            COMMENT "Generating Logical Plan Lexer!"
            BUILD_BYPRODUCTS ${LogicalPlanFlexOutput}
    )
    # linear algebra dsl lexer commands
    ADD_CUSTOM_COMMAND(
            OUTPUT ${LinearAlgebraDSLFlexOutput}
            COMMAND ${FLEX_EXECUTABLE}
            --outfile=${LinearAlgebraDSLFlexOutput}
            ${CMAKE_SOURCE_DIR}/src/linearAlgebraDSL/source/LALexer.l
            COMMENT "Generating Linear Algebra DSL Lexer!"
            BUILD_BYPRODUCTS ${LinearAlgebraDSLFlexOutput}
    )
ENDIF ()

# generate the parser
FIND_PACKAGE(BISON REQUIRED)

# set the output paths
SET(LogicalPlanParserOutput ${CMAKE_SOURCE_DIR}/src/logicalPlan/source/Parser.c)
SET(LinearAlgebraDSLParserOutput ${CMAKE_SOURCE_DIR}/src/linearAlgebraDSL/source/LAParser.c)

# setup the commands
IF (BISON_FOUND)
    # logical plan parser commands
    ADD_CUSTOM_COMMAND(
            OUTPUT ${LogicalPlanParserOutput}
            COMMAND ${BISON_EXECUTABLE}
            --output=${LogicalPlanParserOutput}
            ${CMAKE_SOURCE_DIR}/src/logicalPlan/source/Parser.y
            COMMENT "Generating Logical Plan Parser"
            BUILD_BYPRODUCTS ${LogicalPlanParserOutput}
    )
    # linear algebra dsl parser commands
    ADD_CUSTOM_COMMAND(
            OUTPUT ${LinearAlgebraDSLParserOutput}
            COMMAND ${BISON_EXECUTABLE}
            --output=${LinearAlgebraDSLParserOutput}
            ${CMAKE_SOURCE_DIR}/src/linearAlgebraDSL/source/LAParser.y
            COMMENT "Generating Linear Algebra DSL Parser"
            BUILD_BYPRODUCTS ${LinearAlgebraDSLParserOutput}
    )
ENDIF ()

# setup the command to generate the object ids
SET(BUILT_IN_OBJECT_TYPE_ID ${CMAKE_SOURCE_DIR}/src/objectModel/headers/BuiltInObjectTypeIDs.h)
ADD_CUSTOM_COMMAND(OUTPUT "${BUILT_IN_OBJECT_TYPE_ID}"
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/generateTypeCodes.py" BuiltInObjectTypeIDs
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating type codes!"
        BUILD_BYPRODUCTS "${BUILT_IN_OBJECT_TYPE_ID}")

# setup the command to generate the object ids
SET(BUILT_IN_PDB_OBJECTS ${CMAKE_SOURCE_DIR}/src/objectModel/headers/BuiltinPDBObjects.h)
SET(BUILT_IN_PDB_OBJECTS_GENERATED ${CMAKE_SOURCE_DIR}/src/objectModel/headers/BuiltinPDBObjects.h
                                   ${CMAKE_SOURCE_DIR}/src/objectModel/headers/BuiltinPDBObjects.cc)
ADD_CUSTOM_COMMAND(
        OUTPUT "${BUILT_IN_PDB_OBJECTS}"
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/generateTypeCodes.py" BuiltinPDBObjects
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating type codes!"
        BUILD_BYPRODUCTS "${BUILT_IN_PDB_OBJECTS_GENERATED}"
)

# find the threads library
find_package(Threads)

# setup the libraries
add_library(sql-lite OBJECT ${SOURCE_FILES_SQL_LITE})
add_library(server-functionalities OBJECT ${SERVER_FUNCTIONALITIES_SOURCE} ${BUILT_IN_OBJECT_TYPE_ID})
add_library(communication OBJECT ${COMMUNICATION_SOURCE} ${BUILT_IN_OBJECT_TYPE_ID})
add_library(catalog OBJECT ${CATALOG_SOURCE})
add_library(dispatcher OBJECT ${DISPATCHER_SOURCE})
add_library(lib-pdb-server OBJECT ${PDB_SERVER_SOURCE} ${BUILT_IN_OBJECT_TYPE_ID})
add_library(query-execution OBJECT ${QUERY_EXECUTION_SOURCE})
add_library(query-planning OBJECT ${QUERY_PANNING_SOURCE})
add_library(query-intermediary-rep OBJECT ${QUERY_INTERMEDIARY_REP_SOURCE})
add_library(work OBJECT ${WORK_SOURCE})
add_library(memory OBJECT ${MEMORY_SOURCE})
add_library(storage OBJECT ${STORAGE_SOURCE})
add_library(distribution-manager OBJECT ${DISTRIBUTION_MANAGER_SOURCE})
add_library(lambdas OBJECT ${LAMBDAS_SOURCE})
add_library(logical-plan OBJECT ${LOGICAL_PLAN_SOURCE})
add_library(linear-algebra OBJECT ${LINEAR_ALGEBRA_DSL_SOURCE})
add_library(boost-filesystem OBJECT ${BOOST_FILESYSTEM_SOURCE})
add_library(program-options OBJECT ${PROGRAM_OPTIONS_SOURCE} ${BUILT_IN_OBJECT_TYPE_ID} ${BUILT_IN_PDB_OBJECTS})
add_library(system OBJECT ${SYSTEM_SOURCE})
add_library(object-model OBJECT ${OBJECT_MODEL_SOURCE} ${BUILT_IN_OBJECT_TYPE_ID} ${BUILT_IN_PDB_OBJECTS})
add_library(pdb-shared $<TARGET_OBJECTS:sql-lite>
        $<TARGET_OBJECTS:server-functionalities>
        $<TARGET_OBJECTS:communication>
        $<TARGET_OBJECTS:catalog>
        $<TARGET_OBJECTS:dispatcher>
        $<TARGET_OBJECTS:lib-pdb-server>
        $<TARGET_OBJECTS:object-model>
        $<TARGET_OBJECTS:query-execution>
        $<TARGET_OBJECTS:query-planning>
        $<TARGET_OBJECTS:query-intermediary-rep>
        $<TARGET_OBJECTS:work>
        $<TARGET_OBJECTS:memory>
        $<TARGET_OBJECTS:storage>
        $<TARGET_OBJECTS:distribution-manager>
        $<TARGET_OBJECTS:lambdas>
        $<TARGET_OBJECTS:logical-plan>
        $<TARGET_OBJECTS:linear-algebra>
        $<TARGET_OBJECTS:boost-filesystem>
        $<TARGET_OBJECTS:program-options>
        $<TARGET_OBJECTS:system>)

# form the parsers for the logical plan and linear algebra
add_library(logical-plan-parser OBJECT ${LogicalPlanFlexOutput} ${LogicalPlanParserOutput})
add_library(linear-algebra-parser OBJECT ${LinearAlgebraDSLFlexOutput} ${LinearAlgebraDSLParserOutput} ${BUILT_IN_OBJECT_TYPE_ID} ${BUILT_IN_PDB_OBJECTS})

# set the pdb-cluster build target
set(SOURCE_FILES_PDB_CLUSTER ${PROJECT_SOURCE_DIR}/src/mainServer/source/MasterMain.cc)
add_executable(pdb-cluster ${SOURCE_FILES_PDB_CLUSTER} $<TARGET_OBJECTS:logical-plan-parser> $<TARGET_OBJECTS:linear-algebra-parser>)
target_link_libraries(pdb-cluster pdb-shared)
target_link_libraries(pdb-cluster ${CMAKE_DL_LIBS})
target_link_libraries(pdb-cluster ${CMAKE_THREAD_LIBS_INIT})

# set the pdb-server build target
set(SOURCE_FILES_PDB_CLUSTER ${PROJECT_SOURCE_DIR}/src/mainServer/source/WorkerMain.cc $<TARGET_OBJECTS:logical-plan-parser> $<TARGET_OBJECTS:linear-algebra-parser>)
add_executable(pdb-server ${SOURCE_FILES_PDB_CLUSTER})
target_link_libraries(pdb-server pdb-shared)
target_link_libraries(pdb-server ${CMAKE_DL_LIBS})
target_link_libraries(pdb-server ${CMAKE_THREAD_LIBS_INIT})

# set the source for the pdb-cluster
set(SOURCE_FILES_PDB_CLUSTER ${PROJECT_SOURCE_DIR}/src/tests/source/CatalogServerTests.cc $<TARGET_OBJECTS:logical-plan-parser> $<TARGET_OBJECTS:linear-algebra-parser>)
add_executable(catalog-server-tests ${SOURCE_FILES_PDB_CLUSTER})
target_link_libraries(catalog-server-tests pdb-shared)
target_link_libraries(catalog-server-tests ${CMAKE_DL_LIBS})
target_link_libraries(catalog-server-tests ${CMAKE_THREAD_LIBS_INIT})

# set the source for te catalog-tests
set(SOURCE_FILES_CATALOG_TESTS ${PROJECT_SOURCE_DIR}/src/tests/source/CatalogTests.cc $<TARGET_OBJECTS:logical-plan-parser> $<TARGET_OBJECTS:linear-algebra-parser>)
add_executable(catalog-tests ${SOURCE_FILES_CATALOG_TESTS})
target_link_libraries(catalog-tests pdb-shared)
target_link_libraries(catalog-tests ${CMAKE_DL_LIBS})
target_link_libraries(catalog-tests ${CMAKE_THREAD_LIBS_INIT})

# set the source for te selection-tests
set(SOURCE_FILES_SELECTION_TEST ${PROJECT_SOURCE_DIR}/src/tests/source/Test52.cc $<TARGET_OBJECTS:logical-plan-parser> $<TARGET_OBJECTS:linear-algebra-parser>)
add_executable(selection-test ${SOURCE_FILES_SELECTION_TEST})
target_link_libraries(selection-test pdb-shared)
target_link_libraries(selection-test ${CMAKE_DL_LIBS})
target_link_libraries(selection-test ${CMAKE_THREAD_LIBS_INIT})

# set the source for te sily-join test
set(SOURCE_FILES_SELECTION_TEST ${PROJECT_SOURCE_DIR}/src/tests/source/Test94.cc $<TARGET_OBJECTS:logical-plan-parser> $<TARGET_OBJECTS:linear-algebra-parser>)
add_executable(optimized-join-test ${SOURCE_FILES_SELECTION_TEST})
target_link_libraries(optimized-join-test pdb-shared)
target_link_libraries(optimized-join-test ${CMAKE_DL_LIBS})
target_link_libraries(optimized-join-test ${CMAKE_THREAD_LIBS_INIT})

# compile all the objects
file(GLOB files "${PROJECT_SOURCE_DIR}/src/sharedLibraries/source/*.cc")
foreach(file ${files})
    get_filename_component(fileName "${file}" NAME_WE)
    add_library(${fileName} MODULE ${file})
    target_link_libraries(${fileName} PUBLIC pdb-shared)
    target_link_libraries(${fileName} PUBLIC ${CMAKE_DL_LIBS})
    target_link_libraries(${fileName} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
endforeach()