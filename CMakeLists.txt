cmake_minimum_required(VERSION 3.13)
project(git-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

find_package(ZLIB REQUIRED) # Find the zlib package
find_package(OpenSSL REQUIRED) # Find the OpenSSL package

set(SOURCE_FILES src/Server.cpp)

add_executable(server ${SOURCE_FILES})

if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS}) # Include the zlib directories
    target_link_libraries(server ${ZLIB_LIBRARIES}) # Link the zlib libraries to your executable
endif()

if(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR}) # Include the OpenSSL directories
    target_link_libraries(server ${OPENSSL_LIBRARIES}) # Link the OpenSSL libraries to your executable
endif()