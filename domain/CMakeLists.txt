cmake_minimum_required(VERSION 2.8)

project(domain)

file(GLOB DOMAIN_SRCS src/*.cpp src/*.h)

add_executable(domain ${DOMAIN_SRCS})