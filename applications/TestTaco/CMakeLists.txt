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
            $<TARGET_OBJECTS:logical-plan-parser>)

    target_link_libraries(${main-name} pdb-tests-common)
    target_link_libraries(${main-name} ${taco})
    target_link_libraries(${main-name} "/home/dcbdan/Ps/toTra/dist/build/toTraForeign/libtoTraForeign.so")
    # The rts version of haskell is needed, as it contains all the haskell things that the haskell .so
    # doesn't contain
    target_link_libraries(${main-name} "/home/dcbdan/.local/lib/ghc-8.10.2/rts/libHSrts-ghc8.10.2.so")
    #add_dependencies(build-integration-tests ${main-name})

    add_dependencies(${main-name} ta-shared-libraries)
endfunction(add_testtaco_main_hs)

add_testtaco_main_hs(TestHs)
