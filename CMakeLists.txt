cmake_minimum_required(VERSION 3.16)
project(pulse-qt VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

qt_standard_project_setup()

# ── jsonview 共享库（复用 json-viewer 的 JsonTreeModel/Delegate）──
add_subdirectory(libs/json-view)

qt_add_executable(pulse-qt
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/RequestPanel.cpp
    src/RequestPanel.h
    src/ResponsePanel.cpp
    src/ResponsePanel.h
    src/EnvironmentStore.cpp
    src/EnvironmentStore.h
    src/CollectionStore.cpp
    src/CollectionStore.h
    src/CollectionTree.cpp
    src/CollectionTree.h
    src/EnvironmentDialog.cpp
    src/EnvironmentDialog.h
    src/SaveToCollectionDialog.cpp
    src/SaveToCollectionDialog.h
    src/HistoryStore.cpp
    src/HistoryStore.h
    src/HistoryList.cpp
    src/HistoryList.h
)

target_link_libraries(pulse-qt PRIVATE jsonview Qt6::Widgets Qt6::Network Qt6::Sql)

target_compile_definitions(pulse-qt PRIVATE APP_VERSION="${PROJECT_VERSION}")
