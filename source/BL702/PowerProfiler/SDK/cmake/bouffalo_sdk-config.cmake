if(NOT DEFINED ENV{BL_SDK_BASE})
    message( "please set BL_SDK_BASE in your system environment")
endif()

set(BL_SDK_BASE $ENV{BL_SDK_BASE})

set(build_dir ${CMAKE_CURRENT_BINARY_DIR}/build_out)
set(PROJECT_SOURCE_DIR ${BL_SDK_BASE})
set(PROJECT_BINARY_DIR ${build_dir})
set(EXECUTABLE_OUTPUT_PATH ${build_dir})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(sdk_intf_lib INTERFACE)
add_library(app STATIC ${BL_SDK_BASE}/bsp/board/${BOARD}/board.c)
target_link_libraries(app sdk_intf_lib)

include(${BL_SDK_BASE}/cmake/toolchain.cmake)
include(${BL_SDK_BASE}/cmake/extension.cmake)
include(${BL_SDK_BASE}/cmake/compiler_flags.cmake)

enable_language(C CXX ASM)

add_subdirectory(${BL_SDK_BASE} ${build_dir})