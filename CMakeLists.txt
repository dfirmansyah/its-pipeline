# Minimum required version for modern CMake practices
cmake_minimum_required(VERSION 3.10)

# Project definition
project(its_pipeline CXX)

# Set the C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ZLIB_BUILD_EXAMPLES False)

# Force static linking of MinGW runtime for all targets 
if(MINGW) 
    message(STATUS ">>> Enabling full static linking for MinGW build") 
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static") 
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static") 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -pthread") 
endif()

set(ROOTDIR "${CMAKE_SOURCE_DIR}")

add_subdirectory(${ROOTDIR}/libs/zlib-1.3.1)

set(APP_INCLUDES
    "${ROOTDIR}/libs/zlib-1.3.1"
)

# Source files
file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp")

# add_library(its_pipeline STATIC ${SRC_FILES})

# add_executable(its_pipeline "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_executable(its_pipeline
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
    ${SRC_FILES}
)

target_include_directories(its_pipeline
    PUBLIC
        ${APP_INCLUDES}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(WIN32)
    message(STATUS "Compiling for Windows (MinGW/MSVC)")
    target_link_libraries(its_pipeline PRIVATE 
        ws2_32 zlib
    )
else()
    message(STATUS "Compiling for Windows (MinGW/MSVC)")
    target_link_libraries(its_pipeline PRIVATE 
        zlib
    )
endif()