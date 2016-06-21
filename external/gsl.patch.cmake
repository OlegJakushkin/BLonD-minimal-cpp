if(EXISTS  ${PROJECT_SOURCE_DIR}/${GSL_ROOT}/CMakeLists.txt)
    message(STATUS "already patched")
else() 
    message(STATUS "patching...")
    execute_process(COMMAND  git apply -v ${PROJECT_SOURCE_DIR}/external/gsl.diff )
    execute_process(COMMAND  cmake -E copy ${PROJECT_SOURCE_DIR}/${GSL_ROOT}/contrib/CMakeLists.txt ${PROJECT_SOURCE_DIR}/${GSL_ROOT}/CMakeLists.txt )
endif()