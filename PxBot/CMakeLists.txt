cmake_minimum_required(VERSION 3.15)
project(PxBot)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include vcpkg (adjust the path if necessary)
set(CMAKE_TOOLCHAIN_FILE "C:/Users/Leo/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
include_directories("C:/downloads/opencv/build/include")

# Add executable
add_executable(PxBot main.cpp)

## OpenCV ##
find_package(OpenCV REQUIRED PATHS "C:/downloads/opencv/build/x64/vc16" NO_DEFAULT_PATH)
target_link_libraries(PxBot PRIVATE ${OpenCV_LIBS})
link_directories("C:/downloads/opencv/build/x64/vc16/lib")

## nlohmann_json ##
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(PxBot PRIVATE nlohmann_json::nlohmann_json)

## Qt6 ##
set(Qt6_DIR "C:/Qt/6.8.1/mingw_64/lib/cmake/Qt6")
set(CMAKE_C_COMPILER "C:/Qt/Tools/mingw1310_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/Qt/Tools/mingw1310_64/bin/g++.exe")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
target_link_libraries(PxBot PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)


# Link the necessary libraries for a Win32 app
target_link_libraries(PxBot PRIVATE user32)