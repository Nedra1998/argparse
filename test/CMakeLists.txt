file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG devel)
FetchContent_MakeAvailable(Catch2)

add_executable(unit-test ${SOURCES})
target_link_libraries(unit-test PRIVATE ${PROJECT_NAME}::${PROJECT_NAME}
                                        Catch2::Catch2WithMain)

include(EnableExtraCompilerWarnings)
enable_extra_compiler_warnings(unit-test)


include(Catch)
catch_discover_tests(unit-test)
