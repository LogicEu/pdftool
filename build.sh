#!/bin/bash

comp=gcc
name=libpdftool
src=*.c

flags=(
    -std=c99
    -Wall
    -Wextra
    -O2
)

inc=(
    -I.
)

lib=(
    -lz
)

dlib() {
    $comp ${flags[*]} ${inc[*]} ${lib[*]} -dynamiclib $src -o $name.dylib
    install_name_tool -id @executable_path/$name.dylib $name.dylib
}

slib() {
    $comp ${flags[*]} ${inc[*]} -c ${src[*]}
    rm $name.a 
    ar -cvq $name.a *.o
    rm *.o
}

fail() {
    echo "Run with -d to build dynamically, or -s to build statically."
    exit
}

if [[ $# < 1 ]]; then
    fail
elif [[ "$1" == "-d" ]]; then
    dlib
elif [[ "$1" == "-s" ]]; then
    slib
else
    fail
fi
