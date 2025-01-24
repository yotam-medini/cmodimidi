cmake_minimum_required(VERSION 3.10)
project(modimidi)

# Find the Boost libraries
find_package(Boost COMPONENTS program_options REQUIRED)

# Find fmt library (if available)
find_package(fmt REQUIRED)

# Find the fluidsynth library (adjust paths as needed)
find_library(FLUIDSYNTH_LIBRARY NAMES fluidsynth PATHS /usr/lib /usr/lib/x86_64-linux-gnu)
if(NOT FLUIDSYNTH_LIBRARY)
    message(FATAL_ERROR "Could not find the fluidsynth library. Please check the installation and paths.")
endif()

# Set the include directories
# /usr/include/fluidsynth 
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${FMT_INCLUDE_DIRS}
    /usr/include 
)

# Set the source files
set(SOURCE_FILES
    main.cpp
    dump.cpp
    midi.cpp
    options.cpp
    play.cpp
    synthseq.cpp
    util.cpp
)

# Add the executable target
add_executable(modimidi ${SOURCE_FILES})

# Link with the required libraries
# ${FMT_LIBRARIES}
target_link_libraries(modimidi 
    ${FLUIDSYNTH_LIBRARY} 
    ${Boost_LIBRARIES}
    fmt::fmt-header-only
)
