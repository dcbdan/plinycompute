# TODO: remove this hardcode
#set(TACO_INCLUDE_DIR "/home/dcbdnl/Ps/taco/include")
#set(TACO_LIBRARY_DIR "/home/dcbdnl/Ps/taco/build/lib")
#find_library(taco taco ${TACO_LIBRARY_DIR})

## create the target and make it work with taco
add_pdb_integration_test(TestTaco)
#include_directories(${TACO_INCLUDE_DIR})
target_link_libraries(TestTaco ${taco})

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

