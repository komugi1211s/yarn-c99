@echo off

IF NOT EXIST dist (
    rem "[Build]: Making dist directory."
    mkdir dist
)

setlocal

set CLFLAGS=/Zi /W2 /WX /Fo"./dist/" /Fd"./dist/"
set LFLAGS=/INCREMENTAL:NO /pdb:"./dist/" /out:"./dist/main.exe"
cl.exe %CLFLAGS% ./src/main.c /link %LFLAGS%

endlocal
