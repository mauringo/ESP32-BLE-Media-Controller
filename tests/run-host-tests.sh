#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

for test_source in \
    hid_connection_test.cpp \
    idle_timer_test.cpp \
    pairing_hold_test.cpp
do
    test_name=${test_source%.cpp}
    c++ -std=c++17 -Wall -Wextra -Werror \
        -I"$project_dir/include" \
        -I"$project_dir/ESP32-BLE-Keyboard-master" \
        "$project_dir/tests/$test_source" \
        -o "$tmp_dir/$test_name"
    "$tmp_dir/$test_name"
    printf 'PASS %s\n' "$test_name"
done
