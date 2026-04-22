cmake_minimum_required(VERSION 3.20)
project(hotel_booking_pg_userver CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(userver REQUIRED COMPONENTS postgresql)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/security.cpp
    src/token_store.cpp
    src/handlers.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    userver::postgresql
    OpenSSL::Crypto
)
