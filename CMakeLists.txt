cmake_minimum_required(VERSION 3.15)
project(OllamaAgent VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)

# Common source files (shared between CLI and GUI)
set(COMMON_SOURCES
    src/agent.cpp
    src/file_manager.cpp
    src/json_parser.cpp
    src/ollama_client.cpp
)

# CLI executable
add_executable(ollama_agent 
    src/main.cpp
    ${COMMON_SOURCES}
)

target_include_directories(ollama_agent PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(ollama_agent PRIVATE 
    ${CURL_LIBRARIES}
)

# GUI executable (Windows only)
if(WIN32)
    add_executable(ollama_agent_gui WIN32
        src/gui_main.cpp
        ${COMMON_SOURCES}
    )

    target_include_directories(ollama_agent_gui PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CURL_INCLUDE_DIRS}
    )

    target_link_libraries(ollama_agent_gui PRIVATE 
        ${CURL_LIBRARIES}
        ws2_32
        comctl32
        shell32
        ole32
    )

    target_compile_definitions(ollama_agent_gui PRIVATE UNICODE _UNICODE)
endif()

# Windows specific settings
if(WIN32)
    target_link_libraries(ollama_agent PRIVATE ws2_32)
    set_target_properties(ollama_agent PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# Install targets
install(TARGETS ollama_agent DESTINATION bin)
if(WIN32)
    install(TARGETS ollama_agent_gui DESTINATION bin)
endif()
