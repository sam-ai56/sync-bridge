cmake_minimum_required(VERSION 3.12)
project(syncbridge)

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(dotenv-cpp)
add_subdirectory(cpp-httplib)
add_subdirectory(json)

set(SOURCE_FILES
    main.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${CMAKE_PROJECT_NAME} dotenv httplib::httplib nlohmann_json::nlohmann_json)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/.env
        ${CMAKE_CURRENT_BINARY_DIR}/.env)