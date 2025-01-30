#!/bin/bash

g++ -std=c++20 -shared -fPIC -o liblocaletime_override.so override.cpp -ldl