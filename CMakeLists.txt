cmake_minimum_required(VERSION 3.19)
project(URLShortener)
 
set (CMAKE_CXX_STANDARD 20) 

file(GLOB_RECURSE srcs
   "server/*.h"
   "server/*.cpp"
)
  
add_executable(${CMAKE_PROJECT_NAME} ${srcs})


file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

set(AddBoost.cmake_VERSION 3.7.3)
CPMAddPackage(
  NAME AddBoost.cmake
  VERSION "${AddBoost.cmake_VERSION}"
  URL "https://github.com/Arniiiii/AddBoost.cmake/archive/refs/tags/${AddBoost.cmake_VERSION}.tar.gz"
)


set(TRY_BOOST_VERSION "1.88.0.beta1")
set(BOOST_MY_OPTIONS "BOOST_ENABLE_PYTHON ON;")
set(BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED "thread;system")
set(BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED "asio;beast;uuid;container;cobalt;filesystem;json")

add_boost(
  TRY_BOOST_VERSION BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED
  BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED ${PROJECT_NAME}
)


# Добавление libpqxx через CPM.cmake
CPMAddPackage(
  NAME libpqxx
  VERSION 8.0.1
  GIT_REPOSITORY "https://github.com/jtv/libpqxx.git"
  GIT_TAG "8.0.1"
  OPTIONS "PQXX_BUILD_SHARED ON"  # Или OFF для статической библиотеки
)

# Подключение к проекту
target_link_libraries(${CMAKE_PROJECT_NAME} 
  PRIVATE
    pqxx  # libpqxx
)

# Включение директорий include
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  ${libpqxx_SOURCE_DIR}/include
)