cmake_minimum_required(VERSION 3.22)

project(UnusedVarsMarker)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(
  /usr/lib/llvm-17/include
  /usr/lib/llvm-17/tools/clang/include
)

link_directories(/usr/lib/llvm-17/lib)

add_library(UnusedVarsMarker MODULE UnusedVarsMarker.cpp)

set_target_properties(UnusedVarsMarker PROPERTIES
  CXX_STANDARD 17
  PREFIX ""
  SUFFIX ".so"
)

target_link_libraries(UnusedVarsMarker
  PRIVATE
    clangAST
    clangASTMatchers
    clangBasic
    clangFrontend
)
