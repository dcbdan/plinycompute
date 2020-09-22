###
### tensor algebra shared objects
###

add_custom_target(ta-shared-libraries)

include_directories("${PROJECT_SOURCE_DIR}/applications/TestTaco/sharedLibraries/headers")
file(GLOB files "${PROJECT_SOURCE_DIR}/applications/TestTaco/sharedLibraries/source/*.cc")
# compile all the shared library
foreach(file ${files})
    get_filename_component(fileName "${file}" NAME_WE)
    add_library(${fileName} MODULE ${file})
    target_link_libraries(${fileName} pdb-shared-common)
    #target_link_libraries(${fileName} ${GSL_LIBRARIES})
    target_link_libraries(${fileName} ${taco})
    add_dependencies(shared-libraries ${fileName})
    add_dependencies(ta-shared-libraries ${fileName})
endforeach()

###
### tensor algebra dsl interpreter
###

include_directories("${PROJECT_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/headers")
file(GLOB TENSOR_ALGEBRA_DSL_SOURCE "${PROJECT_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/*.cc")

FIND_PACKAGE(BISON REQUIRED)
SET(TensorAlgebraDSLParserOutput ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/NParser.cc)
SET(TensorAlgebraDSLParserHeader ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/headers/NParserGen.h)
IF (BISON_FOUND)
    # linear algebra dsl parser commands
    ADD_CUSTOM_COMMAND(
            OUTPUT ${TensorAlgebraDSLParserOutput}
            COMMAND ${BISON_EXECUTABLE}
            --defines=${TensorAlgebraDSLParserHeader}
            --output=${TensorAlgebraDSLParserOutput}
            ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/NParser.y
            COMMENT "Generating Tensor Algebra DSL Parser"
            BUILD_BYPRODUCTS ${TensorAlgebraDSLParserOutput} ${TensorAlgebraDSLParserHeader}
    )
ENDIF ()

FIND_PACKAGE(FLEX REQUIRED)
SET(TensorAlgebraDSLFlexOutput ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/NLexer.cc)
IF (FLEX_FOUND)
    # linear algebra dsl lexer commands
    ADD_CUSTOM_COMMAND(
            OUTPUT ${TensorAlgebraDSLFlexOutput}
            COMMAND ${FLEX_EXECUTABLE}
            --outfile=${TensorAlgebraDSLFlexOutput}
            ${CMAKE_SOURCE_DIR}/applications/TestTaco/tensorAlgebraDSL/source/NLexer.l
            COMMENT "Generating Tensor Algebra DSL Lexer!"
            BUILD_BYPRODUCTS ${TensorAlgebraDSLFlexOutput}
    )
ENDIF ()

add_library(tensor-algebra OBJECT ${TENSOR_ALGEBRA_DSL_SOURCE} ${TensorAlgebraDSLFlexOutput} ${TensorAlgebraDSLParserOutput})
#add_library(tensor-algebra-parser OBJECT
#add_library(tensor-algebra-hs OBJECT ${

function(add_testtaco_main main-name)
    ## create the target and make it work with taco
    get_filename_component(test-path ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)

    # add the inlcude path
    include_directories("${test-path}/sharedLibraries/headers")
    include_directories("${test-path}/tensorAlgebraDSL/headers")

    # create the target
    add_executable(${main-name} "${test-path}/${main-name}.cc"
            $<TARGET_OBJECTS:logical-plan-parser>
            $<TARGET_OBJECTS:tensor-algebra>)

    target_link_libraries(${main-name} pdb-tests-common)
    target_link_libraries(${main-name} ${taco})
    add_dependencies(build-integration-tests ${main-name})

    add_dependencies(${main-name} ta-shared-libraries)
endfunction(add_testtaco_main)

function(add_testtaco_main_hs main-name)
    ## create the target and make it work with taco
    get_filename_component(test-path ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)

    # add the inlcude path
    include_directories("${test-path}/sharedLibraries/headers")
    # TODO: add CMAKE params for include files + .so files to get Haskell to run
    include_directories("/home/dcbdan/Ps/toTra/dist/build/toTraForeign/toTraForeign-tmp/ToTra")
    ##include_directories("${test-path}/hsInterface")
    # this is needed for HsFFI.h
    include_directories("/usr/lib/ghc/include")

    # create the target
    add_executable(${main-name} "${test-path}/${main-name}.cc"
            $<TARGET_OBJECTS:logical-plan-parser>
            $<TARGET_OBJECTS:tensor-algebra>)

    target_link_libraries(${main-name} pdb-tests-common)
    target_link_libraries(${main-name} ${taco})
    target_link_libraries(${main-name} "/home/dcbdan/Ps/toTra/dist/build/toTraForeign/libtoTraForeign.so")
    # The rts version of haskell is needed, as it contains all the haskell things that the haskell .so
    # doesn't contain
    target_link_libraries(${main-name} "/home/dcbdan/.local/lib/ghc-8.10.2/rts/libHSrts-ghc8.10.2.so")
    #add_dependencies(build-integration-tests ${main-name})

    add_dependencies(${main-name} ta-shared-libraries)
endfunction(add_testtaco_main_hs)

#add_testtaco_main(TestTaco)
add_testtaco_main(TestTacoReadMTX)
add_testtaco_main(TestTacoTest)
add_testtaco_main_hs(TestHs)
