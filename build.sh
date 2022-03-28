if [ ! -d "dist" ]; then
    mkdir dist
fi

ctags ./src/yarn_c99.h
clang -fsanitize=undefined -std=gnu99 -g -o dist/compiled src/main.c -fno-caret-diagnostics
