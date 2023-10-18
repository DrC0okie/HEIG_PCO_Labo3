cmake_minimum_required(VERSION 3.5)

project(PCO_Labo_3 VERSION 0.1 LANGUAGES CXX)

# General settings
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Qt-related MOC, RCC, and UIC file generation
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Generate CXX settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Wsign-conversion -pedantic")

# Configure compiler flags
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
    add_compile_options(-g3 -fPIE -pie)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-DNDEBUG)
    add_compile_options(-Ofast)
endif ()

# Find Qt5. We can assume Qt5 from then on
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Source files
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h ${CMAKE_SOURCE_DIR}/*.ui)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)
