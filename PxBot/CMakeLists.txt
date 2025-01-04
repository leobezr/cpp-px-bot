cmake_minimum_required(VERSION 3.15)
project(PxBot)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(PxBot main.cpp)

# Include vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
include_directories("C:/downloads/opencv/build/include")

# Find the nlohmann_json package
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV REQUIRED PATHS "C:/downloads/opencv/build/x64/vc16" NO_DEFAULT_PATH)

# Add to lib
link_directories("C:/downloads/opencv/build/x64/vc16/lib")
target_link_libraries(PxBot PRIVATE nlohmann_json::nlohmann_json)

# Add executable and link OpenCV libraries
add_executable(PxBot main.cpp)
target_link_libraries(PxBot ${OpenCV_LIBS})