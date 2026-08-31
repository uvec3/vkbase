cmake_minimum_required(VERSION 3.20)
project(vkbase)

find_package(CUDAToolkit REQUIRED)

add_library(vkbase_cuda
        cuda.cpp cuda.hpp
        Buffer.cpp Buffer.hpp
        Image.cpp Image.hpp
        Semaphore.cpp Semaphore.hpp)
target_link_libraries(vkbase_cuda PUBLIC vkbase::core CUDA::cudart)


add_library(vkbase::cuda ALIAS vkbase_cuda)
