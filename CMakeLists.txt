cmake_minimum_required(VERSION 3.6)
project(lurk)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(contrib)

add_library(lurk SHARED
src/dllmain.cpp
src/setup.cpp
src/luahook.cpp
src/settings.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
minhook
${CMAKE_CURRENT_SOURCE_DIR}/lib/lua51.lib)

target_include_directories(${PROJECT_NAME} PRIVATE contrib/luajit/src)
