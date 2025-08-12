#!/bin/bash

#remove previos outputs if any
rm -rf chapters
rm -rf toc_sections
rm -rf chapter_segments

# clean previous binaries
make clean
# run main.cpp
make run
