
# Yarn c99 runtime (WIP).
fairly incomplete.
** currently relies on `protobuf-c` and generated header for `yarn_spinner.proto`. **

## feature:
 - tiny and lightweight (simply include 1 file to a project, and you're done!)
 - Load/parse `yarnc` + `csv` file and produce yarn file. (subject to deprecation once the compiler is done.)
 - register C function to virtual machine, with step similar to lua.
 - provide `yarn_kvmap`, simple generic hashmap with `char *` as a string.
 - provide `yarn_allocator`, simple arena allocator with pointer-persistency.

## TODO:
 - [ ] sensible file structure while keeping it single header.
 - [ ] `.yarn` file compiler.
 - [ ] make it independent from `protobuf-c`.
 - [ ] example for integrating to lua.
 - [ ] example for integrating to wren.

## how to use:
example is in `src/main.c`. there are also `example/simple_raylib_game.c` that relies on `raylib` framework.

1. in any C file, simply include `yarn_c99.h` like any other header.

```c
#include "yarn_c99.h"

// some of your code here...
```

2. inside exactly **ONE** C/C++ file, define `YARN_C99_IMPLEMENTATION` before including `yarn_c99.h`. at this point, you also have to have `yarn_spinner.pb-c.h` included.

```c
#include "yarn_spinner.pb-c.h"
#define YARN_C99_IMPLEMENTATION

#include "yarn_c99.h" // this will trigger an implementation
```

3. follow the usage example below.

```c
    yarn_variable_storage  storage = yarn_create_default_storage();
    yarn_string_table *string_table = yarn_create_string_table();
    yarn_dialogue *dialogue = yarn_create_dialogue(storage);

    yarn_load_program(dialogue, yarnc_bytes, yarnc_bytes_read);
    yarn_load_string_table(string_table, csv_bytes, csv_bytes_read);
    // free(yarnc_bytes); free(csv_bytes);

    dialogue->strings = string_table; // optional
    dialogue->line_handler = your_line_handler;
    dialogue->option_handler = your_option_handler;
    dialogue->command_handler = your_command_handler;
    ...

    do { yarn_continue(dialogue); } while (yarn_is_active(dialogue));

    //  once it's done
    yarn_destroy_dialogue(dialogue);
    yarn_destroy_string_table(string_table);
    yarn_destroy_default_storage(storage);
```

more example / docs are inside `yarn_c99.h`.
