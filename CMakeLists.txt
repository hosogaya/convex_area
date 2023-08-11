cmake_minimum_required(VERSION 3.5)
project(opencv_test)

# For OpenCV
find_package(OpenCV REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${OpenCV_INCLUDE_DIRS}> # Eigen3
)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

file(COPY ${CMAKE_SOURCE_DIR}/resource/ DESTINATION ${PROJECT_BINARY_DIR}/resource)
add_definitions(-DRESOURCE_DIR="${PROJECT_BINARY_DIR}/resource/")