#!/bin/bash
mkdir -p build
./with-macro g++ instagui.cpp -g -c -o build/instagui.o
./with-macro g++ iggtk.cpp -g -c `pkg-config gtkmm-3.0 --cflags` -o build/iggtk.o
./with-macro g++ main.cpp -g -c -o build/main.o
g++ `pkg-config gtkmm-3.0 --libs` -g -lpthread build/* -o a.out
