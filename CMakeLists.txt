cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 14)

# 为 clangd 导出编译选项
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


project(cytmwia_cpp)

add_subdirectory(message_queue)

set(CYTMWIA_CPP
  ${PROJECT_NAME}_message_queue

  CACHE INTERNAL ""
)
