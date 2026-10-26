#!/bin/zsh
set -e
cd "${0:A:h}"
mkdir -p 'Bloch Lab.app/Contents/MacOS'
clang++ -std=c++17 -O2 -arch arm64 -arch x86_64 -mmacosx-version-min=11.0 -Wno-deprecated-declarations main.cpp window.cpp -framework OpenGL -framework GLUT -o 'Bloch Lab.app/Contents/MacOS/BlochLab'
clang++ -std=c++17 -O2 tests.cpp -o quantum_tests
./quantum_tests
echo 'Build OK: Bloch Lab.app'
