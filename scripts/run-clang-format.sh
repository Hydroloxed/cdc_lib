#!/bin/bash
clang-format -i --files=<(find . -name "*.cpp" -not -path "./build*" -or -name "*.h" -not -path "./build*")