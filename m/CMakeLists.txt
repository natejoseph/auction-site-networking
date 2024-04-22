cmake_minimum_required(VERSION 3.20)
project(Server
    VERSION 1.0
    DESCRIPTION "Middleware Server"
    LANGUAGES CXX)

find_package(functions_framework_cpp REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)

add_executable(Server main.cpp middleware/MiddlewareProgram.cpp middleware/MiddlewareProgram.h)
target_compile_features(Server PRIVATE cxx_std_17)
target_link_libraries(Server functions-framework-cpp::framework)
target_link_libraries(Server PUBLIC google-cloud-cpp::storage)

include(GNUInstallDirs)
install(TARGETS Server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})