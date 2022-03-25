if [ ! -d "dist" ]; then
    mkdir dist
fi

clang -std=gnu99 -g -o dist/compiled src/main.c -fno-caret-diagnostics
