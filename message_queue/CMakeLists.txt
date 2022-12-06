add_library(${PROJECT_NAME}_message_queue INTERFACE)

target_include_directories(${PROJECT_NAME}_message_queue
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()

add_executable(test
  tests/test.cc
  tests/message_queue.cc
)

target_compile_options(test PRIVATE -Wall -Wextra -Wpedantic -Werror)

target_link_libraries(test
  ${PROJECT_NAME}_message_queue
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test)
