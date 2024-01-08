cmake_minimum_required(VERSION 3.13)
project(git-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

find_package(ZLIB REQUIRED) # Find the zlib package

set(SOURCE_FILES src/Server.cpp)

# Find path to the Crypto++ headers
find_path(CRYPTOPP_INCLUDE_DIR NAMES cryptopp/cryptlib.h)

# Find the Crypto++ library
find_library(CRYPTOPP_LIBRARY NAMES cryptopp)

# Make sure both the header files and library were found
if (NOT CRYPTOPP_INCLUDE_DIR OR NOT CRYPTOPP_LIBRARY)
    message(FATAL_ERROR "Could not find Crypto++")
endif()

# Add the Crypto++ headers to the include directories
include_directories(${CRYPTOPP_INCLUDE_DIR})

# Add your executable
add_executable(server ${SOURCE_FILES})

if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS}) # Include the zlib directories
    target_link_libraries(server ${ZLIB_LIBRARIES}) # Link the zlib libraries to your executable
endif()

# Link the Crypto++ library to your executable
target_link_libraries(server ${CRYPTOPP_LIBRARY})