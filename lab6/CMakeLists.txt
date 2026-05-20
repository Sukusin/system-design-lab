cmake_minimum_required(VERSION 3.20)
project(hotel_booking_hw06_userver CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(userver REQUIRED COMPONENTS postgresql rabbitmq)
find_package(OpenSSL REQUIRED)

set(COMMON_SOURCES
    src/security.cpp
    src/token_store.cpp
    src/event_publisher.cpp
    src/rabbit_topology.cpp
)

add_executable(hotel_booking_hw06_api_userver
    src/api_main.cpp
    src/handlers.cpp
    ${COMMON_SOURCES}
)

target_include_directories(hotel_booking_hw06_api_userver PRIVATE src)

target_link_libraries(hotel_booking_hw06_api_userver
    PRIVATE
    userver::postgresql
    userver::rabbitmq
    OpenSSL::Crypto
)

add_executable(hotel_booking_hw06_consumer_userver
    src/consumer_main.cpp
    src/event_consumer.cpp
    src/rabbit_topology.cpp
)

target_include_directories(hotel_booking_hw06_consumer_userver PRIVATE src)

target_link_libraries(hotel_booking_hw06_consumer_userver
    PRIVATE
    userver::postgresql
    userver::rabbitmq
)
