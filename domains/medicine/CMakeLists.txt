add_library(feen_pharma STATIC pharma.cpp)
target_include_directories(feen_pharma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(feen_pharma PUBLIC feen)
