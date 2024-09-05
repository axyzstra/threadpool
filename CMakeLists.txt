cmake_minimum_required(VERSION 3.0)
project(POOL)
set(SRC_LIST threadpool.c main.c)
add_executable(app ${SRC_LIST})
