# from here: https://code.qt.io/cgit/qt/qtbase.git/tree/tests/auto/other/modeltest

add_library(modeltest SHARED modeltest.h modeltest.cpp)
target_link_libraries(modeltest PRIVATE Qt5::Core Qt5::Test)
target_include_directories(modeltest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
