cmake_minimum_required (VERSION 3.2.2)

set(EVENTBUS_VERSION_MAJOR 0)
set(EVENTBUS_VERSION_MINOR 1)
set(EVENTBUS_VERSION_PATCH 1)
set(EVENTBUS_VERSION ${EVENTBUS_VERSION_MAJOR}.${EVENTBUS_VERSION_MINOR}.${EVENTBUS_VERSION_PATCH})

project(eventbus VERSION ${EVENTBUS_VERSION} LANGUAGES CXX)

add_library(eventbus INTERFACE)
target_include_directories(eventbus INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

include(CTest)
add_subdirectory(test)

include(FindDoxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(
        docs
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating doxygen project documentation in ${CMAKE_CURRENT_BINARY_DIR}." VERBATIM
    )
else()
    ADD_CUSTOM_TARGET(docs COMMENT "Doxygen not found. Please install doxygen to use this target." VERBATIM)
endif()
