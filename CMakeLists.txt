cmake_minimum_required(VERSION 3.15)
project(grpc_overlay)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    include_directories(/opt/homebrew/opt/libomp/include)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/opt/homebrew/opt/libomp/lib -lomp")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Xpreprocessor -fopenmp")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp")
else()
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

include_directories(
  ${GRPC_INCLUDE_DIRS}
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/shared
  ${CMAKE_SOURCE_DIR}/generated
  /opt/homebrew/include 
)

link_directories(${GRPC_LIBRARY_DIRS} ${PROTOBUF_LIBRARY_DIRS})

file(GLOB PROTO_SRCS generated/*.cc)

add_executable(A_server machine1/A_server.cpp ${PROTO_SRCS})
add_executable(B_server machine1/B_server.cpp ${PROTO_SRCS})
add_executable(C_server machine2/C_server.cpp ${PROTO_SRCS})
add_executable(D_server machine2/D_server.cpp ${PROTO_SRCS})
add_executable(E_server machine2/E_server.cpp ${PROTO_SRCS})

target_link_libraries(A_server ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
target_link_libraries(B_server ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
target_link_libraries(C_server ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
target_link_libraries(D_server ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
target_link_libraries(E_server ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
