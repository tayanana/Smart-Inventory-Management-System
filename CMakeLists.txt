cmake_minimum_required(VERSION 3.10)
project(AutoInventorySystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Core library sources shared by the CLI and the test binary.
set(INVENTORY_SOURCES
    src/Validation.cpp
    src/DataLayer.cpp
    src/InventoryService.cpp
)

add_executable(AutoInventorySystem src/main.cpp ${INVENTORY_SOURCES})
target_include_directories(AutoInventorySystem PRIVATE include)

add_executable(inventory_tests tests/test_inventory.cpp ${INVENTORY_SOURCES})
target_include_directories(inventory_tests PRIVATE include)
target_compile_definitions(inventory_tests PRIVATE
    TEST_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(AutoInventorySystem PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(inventory_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME inventory_tests COMMAND inventory_tests)
add_test(
    NAME smoke_persist
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/smoke_persist.sh" "$<TARGET_FILE:AutoInventorySystem>"
)
