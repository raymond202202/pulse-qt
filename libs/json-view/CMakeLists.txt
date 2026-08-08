add_library(jsonview STATIC
    JsonTreeModel.cpp
    JsonTreeModel.h
    JsonTreeDelegate.cpp
    JsonTreeDelegate.h
)
target_include_directories(jsonview PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(jsonview PUBLIC Qt6::Widgets)
