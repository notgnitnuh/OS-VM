cmake_minimum_required(VERSION 3.1)
project(file_sys VERSION 1.0
                  DESCRIPTION "A file system project"
                  LANGUAGES C CXX)

add_executable(dicpo dicpo.cpp sfs_code/driver.c sfs_code/bitmap.c)
add_executable(dils dils.cpp sfs_code/driver.c sfs_code/bitmap.c)