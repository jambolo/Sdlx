cmake_minimum_required(VERSION 3.21)

# GTest may already be provided by the parent via FetchContent
if(NOT TARGET GTest::gtest)
    find_package(GTest REQUIRED)
endif()
include(GoogleTest)

# Function to create test executables (not named add_test, which is a CMake command)
function(sdlx_add_test test_name source_file)
    add_executable(${test_name} ${source_file})
    set_target_properties(${test_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    if(WIN32)
        target_compile_definitions(${test_name}
            PRIVATE
                NOMINMAX
                WIN32_LEAN_AND_MEAN
                VC_EXTRALEAN
                _CRT_SECURE_NO_WARNINGS
                _SECURE_SCL=0
                _SCL_SECURE_NO_WARNINGS
        )
    endif()

    target_link_libraries(${test_name} 
        PRIVATE 
            ${PROJECT_NAME}::${PROJECT_NAME}
            GTest::gtest
            GTest::gtest_main
    )
    # On Windows, copy the DLLs that the test depends on (if any) next to it, so that it can run. This must come before
    # gtest_discover_tests, which runs the test after it is built.
    if(WIN32)
        add_custom_command(TARGET ${test_name} POST_BUILD
            COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:${test_name}>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:${test_name}>;$<TARGET_FILE_DIR:${test_name}>>"
            COMMAND_EXPAND_LISTS
        )
    endif()

    gtest_discover_tests(${test_name})
    message(STATUS "Added test executable: ${test_name}")
endfunction()

file(GLOB SOURCES "*.cpp")

message(STATUS "Building tests for ${PROJECT_NAME}")

foreach(FILE ${SOURCES})
    get_filename_component(TEST ${FILE} NAME_WE)
    sdlx_add_test("${PROJECT_NAME}_${TEST}" ${FILE})
endforeach()
