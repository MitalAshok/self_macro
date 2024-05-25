cmake_minimum_required(VERSION 3.16)
project(self_macro)

add_library(self_macro INTERFACE)
target_sources(self_macro INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include/self_macro.h)
target_include_directories(self_macro INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
set_property(TARGET self_macro PROPERTY CXX_STANDARD 14)

add_executable(self_macro_sample ${CMAKE_CURRENT_LIST_DIR}/sample/sample.cpp)
target_link_libraries(self_macro_sample PRIVATE self_macro)
