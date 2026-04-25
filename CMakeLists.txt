cmake_minimum_required(VERSION 3.12)
project(saberdaq)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -g -fPIC")

# Find and configure ROOT
find_package(ROOT REQUIRED)
if(ROOT_FOUND)
    include_directories(${ROOT_INCLUDE_DIRS})
endif()

# Find and configure Polaris
find_path(POLARIS_INCLUDE_DIR polaris)
#find_path(POLARIS_INCLUDE_DIR NAMES polaris PATHS /usr/local/include /usr/include)
find_library(POLARIS_LIBRARY NAMES polaris)
if(POLARIS_INCLUDE_DIR AND POLARIS_LIBRARY)
    message(STATUS "Found Polaris: ${POLARIS_LIBRARY} at ${POLARIS_INCLUDE_DIR}")
    include_directories(${POLARIS_INCLUDE_DIR}/polaris)
else()
    message(FATAL_ERROR "Polaris library not found")
endif()

# Find and configure HDF5 (direct path)
set(HDF5_INCLUDE_DIRS /usr/local/HDF_Group/HDF5/2.0.0/include /usr/include)
set(HDF5_LIBRARIES /usr/local/HDF_Group/HDF5/2.0.0/lib/libhdf5.so /usr/local/HDF_Group/HDF5/2.0.0/lib/libhdf5_cpp.so /usr/local/HDF_Group/HDF5/2.0.0/lib/libhdf5_hl.so)
include_directories(${HDF5_INCLUDE_DIRS})


# Include local directories
include_directories(h5manager/include include /usr/local/include/polaris)
# bug : above last entry SHOULDN'T be required, but somehow oilaris_include_dir points to /usr/local/bin instead of /usr/local/include
# I am also leaving out analysis directories


# Source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
#    "analysis/src/*.cpp"
    "h5manager/src/*.cpp"
)

# Create shared library
add_library(saberdaq SHARED ${SOURCES})

# Linker flags
target_link_libraries(saberdaq
    ${POLARIS_LIBRARY}
    CAENVME
    ${HDF5_LIBRARIES}
    ${ROOT_LIBRARIES}
)

# Executables (commented out)
# set(EXE_FILES
#     "saber-update-format"
#     "saber-convert-hdf5"
# )

# if(ROOT_FOUND)
#     list(APPEND EXE_FILES
#         "saber-event-viewer"
#         "saber-spec-generator"
#         "saber-avg-waveform"
#     )
# endif()

# foreach(exe ${EXE_FILES})
#     add_executable(${exe} exe/${exe}.cpp ${SOURCES})
#     target_link_libraries(${exe} saberdaq)
# endforeach()

# Installation rules
install(TARGETS saberdaq DESTINATION /usr/lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION /usr/include/saberdaq)
# install(TARGETS ${EXE_FILES} DESTINATION /usr/bin)

# Clean target
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}"
    COMMENT "Cleaning everything..."
)

