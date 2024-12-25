cmake_minimum_required(VERSION 3.10)

project(subdivide VERSION 1.0)
include(FetchContent)

# set your directory pathes below
set(LXSDK_PATH "../LXSDK-000025")
set(OPENSUBDIV_PATH "/Users/taz/local")

if (NOT LXSDK_PATH)
    FetchContent_Declare( 
        lxsdk
        URL https://thefoundry.s3.us-east-1.amazonaws.com/products/modo/16.1v5/lxsdk16.1v5_692025.zip
    )
    FetchContent_Populate(lxsdk)
    message(STATUS "FetchContent lxsdk directory: ${lxsdk_SOURCE_DIR}")
    set(LXSDK_PATH ${lxsdk_SOURCE_DIR})
endif()

# Get all source and headers for lxsdk
file(GLOB LXSDK_SOURCES ${LXSDK_PATH}/common/*.cpp)
file(GLOB LXSDK_HEADERS ${LXSDK_PATH}/include/*.h?)

add_library(lxsdk
    STATIC
        ${LXSDK_SOURCES}
)

set_target_properties(lxsdk 
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY lib
        POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(lxsdk
    PRIVATE
        GL_SILENCE_DEPRECATION
)

target_compile_features(lxsdk
    PRIVATE
        cxx_std_17
)

target_include_directories(lxsdk
    PRIVATE
        ${LXSDK_PATH}/include
)

add_library(subdivide
    SHARED
        "source/meshop.cpp"
        "source/initialize.cpp"
        "source/subdivide.cpp"
)

target_compile_definitions(subdivide
    PRIVATE
        GL_SILENCE_DEPRECATION
)

target_compile_features(subdivide
    PRIVATE
        cxx_std_17
)

# Set your LXSDK include directry
target_include_directories(subdivide 
    PUBLIC
        ${LXSDK_PATH}/include
    	${OPENSUBDIV_PATH}/include
)

target_link_directories(subdivide 
    PRIVATE
    	${OPENSUBDIV_PATH}/lib
)

target_compile_options(subdivide
    PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang>:
            -Wno-deprecated-declarations
            -Wno-error
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /bigobj
        >
)

find_package(OpenGL REQUIRED)

# Set your OpenSubdiv libraray
target_link_libraries(subdivide
    PUBLIC
	    lxsdk
    PRIVATE
        osdCPU osdGPU
        OpenGL::GL
)

# Install plugins and configs into kit directry
if (NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/OpenSubdiv_Kit")
endif()
install(
    FILES
        "index.cfg"
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}
)
install(
    TARGETS
        subdivide
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/extra
)
