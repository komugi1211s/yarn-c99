if [ ! -d "dist" ]; then
    mkdir dist
fi

if [ "$1" == "release" ]; then
    echo "[build]: release mode."
    CFLAGS="-O1"
    EXEC_FILE_NAME="dist/compiled_release"
else
    echo "[build]: debug mode."
    CFLAGS="-fsanitize=undefined -g"
    EXEC_FILE_NAME="dist/compiled"
fi

ctags ./src/yarn_c99.h
clang $CFLAGS -std=c99 -Wall -o $EXEC_FILE_NAME src/main.c -fno-caret-diagnostics
