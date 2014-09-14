cmake_minimum_required(VERSION 2.8)

project( gcode-delta )

SET(PROJECT_VERSION_MAJOR 0)
SET(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

SET(TARGET_LIB "lib_gdelta.so")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g -Wno-format-extra-args")
SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -g")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_SOURCE_DIR}/cmake/Modules/")

SET(COMPILE_DEFINITIONS -Werror)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")

get_property(dirs DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
message("INCLUDE_DIRECTORIES:${dirs}")

SET(BUILD_TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/target" CACHE STRING "Put all binary output in target")
SET(LIBRARY_OUTPUT_PATH "${BUILD_TARGET_DIR}")
SET(EXECUTABLE_OUTPUT_PATH "${BUILD_TARGET_DIR}")
SET(RUNTIME_OUTPUT_NAME gdelta)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include_directories( ${PROJECT_SOURCE_DIR} ${BUILD_TARGET_DIR} ${JANSSON_INCLUDE_DIRS} )
include_directories( /usr/local/include )
include_directories( /usr/include )

include (InstallRequiredSystemLibraries)
SET(CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_GENERATED_WARNING "GENERATED FILE!!! DO NOT CHANGE!!!")
SET(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
include (CPack)

get_property(dirs DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
message("INCLUDE_DIRECTORIES:")
foreach(dir ${dirs})
  message(STATUS "  dir='${dir}'")
endforeach()

configure_file (
  "${PROJECT_SOURCE_DIR}/version.h.in"
  "${BUILD_TARGET_DIR}/version.h"
  )

message("CMAKE_PREFIX_PATH:${CMAKE_PREFIX_PATH}")

link_directories( ${BUILD_TARGET_DIR} /usr/lib /usr/local/lib )

set(TARGET_LIB_FILES
  FireLog.cpp 
  gdelta.cpp
  )

add_library(_gdelta SHARED ${TARGET_LIB_FILES})
target_link_libraries(_gdelta ${JANSSON_LIB} )
set_target_properties(_gdelta PROPERTIES 
    VERSION ${PROJECT_VERSION_STRING} 
    SOVERSION ${PROJECT_VERSION_MAJOR}
    )
  
add_executable(gdelta gdelta_main.cpp)
add_dependencies(gdelta _gdelta)
target_link_libraries(gdelta ${JANSSON_LIB} ${TARGET_LIB} )

# add_executable(test 
# test/test.cpp)
#add_dependencies(test _gdelta)
#target_link_libraries(test ${TARGET_LIB} )

#
# Installation preparation.
#

# Allow the user to override installation directories.
set(TARGET_INSTALL_LIB_DIR       lib CACHE PATH "Installation directory for libraries")
set(TARGET_INSTALL_BIN_DIR       bin CACHE PATH "Installation directory for executables")
set(TARGET_INSTALL_INCLUDE_DIR   include CACHE PATH "Installation directory for header files")


INSTALL(TARGETS _gdelta DESTINATION ${TARGET_INSTALL_LIB_DIR})
INSTALL(TARGETS gdelta DESTINATION ${TARGET_INSTALL_BIN_DIR})
INSTALL(FILES FireLog.h gdelta.hpp DESTINATION ${TARGET_INSTALL_INCLUDE_DIR})

