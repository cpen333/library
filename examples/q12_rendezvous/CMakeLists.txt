cmake_minimum_required(VERSION 3.1.3)

# Name project based on current directory
get_filename_component(PROJECT ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT})

# customize output directory
set(MY_OUTPUT_DIR ${MY_OUTPUT_DIR}/${PROJECT})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${MY_OUTPUT_DIR})

set(CMAKE_CXX_STANDARD 11)
include(../../config/Macros.cmake)

# directories to search for header files
include_directories(../../include)

#==============  Rendezvous Process ===============================
add_process_executable(${PROJECT}_process_person person process src/process/person.cpp)
add_process_executable(${PROJECT}_process_roller_coaster roller_coaster process src/process/roller_coaster.cpp)
add_dependencies(${PROJECT}_process_roller_coaster ${PROJECT}_process_person)
install(TARGETS ${PROJECT}_process_roller_coaster ${PROJECT}_process_person
        DESTINATION bin/${MY_OUTPUT_DIR}/process)

#==============  Rendezvous Thread ===============================
add_thread_executable(${PROJECT}_thread_roller_coaster roller_coaster thread src/thread/roller_coaster.cpp)
install(TARGETS ${PROJECT}_thread_roller_coaster
        DESTINATION bin/${MY_OUTPUT_DIR}/thread)
