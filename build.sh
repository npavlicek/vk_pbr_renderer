#!/bin/bash

# Compile shaders
glslc shaders/shader.frag -o shaders/frag.spv
glslc shaders/shader.vert -o shaders/vert.spv

# Compile the application
cmake --build build -j 8
