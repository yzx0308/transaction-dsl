FILE(GLOB_RECURSE all_files
*.cpp
*.cc
*.c++
*.c
*.C)

include_directories(../../cub/include ../../event/include)

add_library(trans-dsl STATIC ${all_files})
target_link_libraries(trans-dsl event cub)

install(TARGETS trans-dsl ARCHIVE DESTINATION lib)
