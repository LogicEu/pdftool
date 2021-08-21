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

mac_lib() {
    $comp ${flags[*]} ${inc[*]} ${lib[*]} -dynamiclib $src -o $name.dylib &&\
    install_name_tool -id @executable_path/$name.dylib $name.dylib
}

linux_lib() {
    $comp -o $name.so $src ${flags[*]} ${inc[*]} ${lib[*]} -shared -fPIC
}

dlib() {
    if echo "$OSTYPE" | grep -q "darwin"; then
        mac_lib
    elif echo "$OSTYPE" | grep -q "linux"; then
        linux_lib
    else
        echo "OS not supported by this script" && exit
    fi
}

slib() {
    $comp ${flags[*]} ${inc[*]} -c $src && ar -cvq $name.a *.o && rm *.o
}

fail() {
    echo "Run with -d to build dynamically, or -s to build statically."
    exit
}

case "$1" in
    "-d")
        dlib;;
    "-s")
        slib;;
    *)
        fail;;
esac
