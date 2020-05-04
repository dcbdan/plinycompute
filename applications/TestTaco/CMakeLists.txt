###
### tensor algebra dsl interpreter
###

include_directories("${PROJECT_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/headers")
file(GLOB TENSOR_ALGEBRA_DSL_SOURCE "${PROJECT_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/*.cc")


FIND_PACKAGE(BISON REQUIRED)
SET(TensorAlgebraDSLParserOutput ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/TParser.cc)
SET(TensorAlgebraDSLParserHeader ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/headers/TParserGen.h)
IF (BISON_FOUND)
    # linear algebra dsl parser commands
    ADD_CUSTOM_COMMAND(
            OUTPUT ${TensorAlgebraDSLParserOutput}
            COMMAND ${BISON_EXECUTABLE}
            --defines=${TensorAlgebraDSLParserHeader}
            --output=${TensorAlgebraDSLParserOutput}
            ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/TParser.y
            COMMENT "Generating Tensor Algebra DSL Parser"
            BUILD_BYPRODUCTS ${TensorAlgebraDSLParserOutput} ${TensorAlgebraDSLParserHeader}
    )
ENDIF ()


FIND_PACKAGE(FLEX REQUIRED)
SET(TensorAlgebraDSLFlexOutput ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/TLexer.cc)
IF (FLEX_FOUND)
    # linear algebra dsl lexer commands
    ADD_CUSTOM_COMMAND(
            OUTPUT ${TensorAlgebraDSLFlexOutput}
            COMMAND ${FLEX_EXECUTABLE}
            --outfile=${TensorAlgebraDSLFlexOutput}
            ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/TLexer.l
            COMMENT "Generating Tensor Algebra DSL Lexer!"
            BUILD_BYPRODUCTS ${TensorAlgebraDSLFlexOutput}
    )
ENDIF ()

add_library(tensor-algebra OBJECT ${TENSOR_ALGEBRA_DSL_SOURCE})
add_library(tensor-algebra-parser OBJECT ${TensorAlgebraDSLFlexOutput} ${TensorAlgebraDSLParserOutput})

## create the target and make it work with taco
get_filename_component(test-path ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)

# add the inlcude path
include_directories("${test-path}/sharedLibraries/headers")

# create the target
add_executable(TestTaco "${test-path}/TestTaco.cc"
        $<TARGET_OBJECTS:logical-plan-parser>
        $<TARGET_OBJECTS:tensor-algebra>
        $<TARGET_OBJECTS:tensor-algebra-parser>)

# link it to the required libraries
target_link_libraries(TestTaco pdb-tests-common)
target_link_libraries(TestTaco ${GSL_LIBRARIES})
#include_directories(${TACO_INCLUDE_DIR})
target_link_libraries(TestTaco ${taco})

#add_library(linear-algebra OBJECT ${LINEAR_ALGEBRA_DSL_SOURCE})
#add_library(linear-algebra-parser OBJECT ${LinearAlgebraDSLFlexOutput} ${LinearAlgebraDSLParserOutput} ${BUILT_IN_OBJECT_TYPE_ID} ${BUILT_IN_PDB_OBJECTS})
#

# add a build dependency to build-tests target
add_dependencies(build-integration-tests TestTaco)

# compile all the objects
file(GLOB files "${PROJECT_SOURCE_DIR}/applications/TestTaco/sharedLibraries/source/*.cc")
foreach(file ${files})
    get_filename_component(fileName "${file}" NAME_WE)
    add_library(${fileName} MODULE ${file})
    target_link_libraries(${fileName} pdb-shared-common)
    target_link_libraries(${fileName} ${GSL_LIBRARIES})
    target_link_libraries(${fileName} ${taco})
    add_dependencies(shared-libraries ${fileName})
endforeach()

# add build dependencies to shared libraries it uses
add_dependencies(TestTaco TacoTensor)
add_dependencies(TestTaco Arr)
add_dependencies(TestTaco TacoMemory)
add_dependencies(TestTaco TacoTensorBlock)
add_dependencies(TestTaco TacoTensorBlockMeta)
add_dependencies(TestTaco TacoMatMulAggregation)
add_dependencies(TestTaco TacoMatMulJoin)
add_dependencies(TestTaco TacoScanner)
add_dependencies(TestTaco TacoWriter)



###add_library(TacoAlloc OBJECT "${PROJECT_SOURCE_DIR}/applications/TestTaco/sharedLibraries/source/TacoMemory.cc")

#add_dependencies(TestTaco TacoMemory)

