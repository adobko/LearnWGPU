#!/bin/bash

# emcmake cmake -B build-web
cmake --build build-web -j4
npx serve ./build-web