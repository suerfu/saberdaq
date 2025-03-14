cmake_minimum_required(VERSION 3.10)
project(saberdaq)

set(LIBNAME "libsaberdaq.so")

find_package(polaris REQUIRED)
if(polaris_FOUND)
  target_include_directories(${PROJECT_NAME} PRIVATE ${polaris_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${polaris_LIBRARIES})
endif()


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -fPIC")

# Include directories
include_directories(
    analysis/include
    include
    h5manager/include
    /usr/include/polaris
    /usr/include
    /usr/local/HDF_Group/HDF5/1.14.6/include
)

# Collect source files
file(GLOB CPP_FILES
    "src/*.cpp"
    "analysis/src/*.cpp"
    "h5manager/src/*.cpp"
)

# Create library
add_library(${PROJECT_NAME} SHARED ${CPP_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "saberdaq"
)

# Linker flags
target_link_libraries(${PROJECT_NAME}
    /usr/lib/libCAENVME.so
    polaris
    hdf5 hdf5_hl hdf5_cpp
    pthread dl
)

# Build executables
set(EXE_FILES saber-update-format saber-convert-hdf5)
if(DEFINED ENV{ROOTSYS})
    list(APPEND EXE_FILES saber-event-viewer saber-spec-generator saber-avg-waveform)
    include_directories($ENV{ROOTSYS}/include)
    link_libraries($ENV{ROOTSYS}/lib)
endif()

foreach(EXE ${EXE_FILES})
    add_executable(${EXE} exe/${EXE}.cpp ${CPP_FILES})
    target_link_libraries(${EXE} ${PROJECT_NAME})
endforeach()

# Installation prefix
set(INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Installation directory prefix")

# Install library
install(TARGETS ${PROJECT_NAME} ${EXE_FILES}
    RUNTIME DESTINATION ${INSTALL_PREFIX}/bin
    LIBRARY DESTINATION ${INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${INSTALL_PREFIX}/lib
)
install(DIRECTORY analysis/include DESTINATION ${INSTALL_PREFIX}/include/saberdaq)
install(DIRECTORY include DESTINATION ${INSTALL_PREFIX}/include/saberdaq)

# Uninstall target
add_custom_target(uninstall
    COMMAND xargs rm < install_manifest.txt
    COMMAND rm -rf ${INSTALL_PREFIX}/include/saberdaq
    VERBATIM
)

# Clean target
add_custom_target(clean-all
    COMMAND ${CMAKE_MAKE_PROGRAM} clean
    COMMAND rm -rf ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND rm -rf ${CMAKE_BINARY_DIR}/Makefile
    COMMAND rm -rf ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND rm -rf bin/*
    COMMAND rm -rf lib/${LIBNAME}
    VERBATIM
)

