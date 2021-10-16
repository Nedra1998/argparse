file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(SOURCE ${SOURCES})
  get_filename_component(TARGET "${SOURCE}" NAME_WE)
  add_executable(${TARGET} ${SOURCES})
  target_link_libraries(${TARGET} PRIVATE ${PROJECT_NAME}::${PROJECT_NAME})

  include(EnableExtraCompilerWarnings)
  enable_extra_compiler_warnings(${TARGET})
endforeach()
