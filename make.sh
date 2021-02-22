#!/bin/bash
mkdir -p build
./with-macro g++ instagui.cpp -std=c++17 -g -c -o build/instagui.o
./with-macro g++ iggtk.cpp -std=c++17 -g -c `pkg-config gtkmm-3.0 --cflags` -o build/iggtk.o
./with-macro g++ main.cpp -std=c++17 -g -c -o build/main.o
g++ -g build/* `pkg-config gtkmm-3.0 --libs` -lpthread -o a.out
