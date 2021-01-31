cmake_minimum_required(VERSION 3.17)
project(imgui-sandbox)

add_executable(imgui-sandbox src/main.cpp)

target_compile_features(imgui-sandbox PUBLIC cxx_std_17)

find_package(glad CONFIG REQUIRED)
target_link_libraries(imgui-sandbox PRIVATE glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(imgui-sandbox PRIVATE glfw)

find_package(glm CONFIG REQUIRED)
target_link_libraries(imgui-sandbox PRIVATE glm)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(imgui-sandbox PUBLIC imgui::imgui)

find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(imgui-sandbox PUBLIC yaml-cpp)
