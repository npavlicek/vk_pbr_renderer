#!/bin/bash

# Compile shaders
glslc shaders/shader.frag -o shaders/frag.spv
glslc shaders/shader.vert -o shaders/vert.spv
glslc shaders/skybox.vert -o shaders/skyboxVert.spv
glslc shaders/skybox.frag -o shaders/skyboxFrag.spv

# Compile the application
cmake --build build -j 8
