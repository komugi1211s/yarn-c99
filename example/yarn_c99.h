/*
 ==========================================
 Yarn C99 runtime v0.0
 Compatible with Yarn Spinner 2.0.
 tested to produce the same result as if you used `ysc run [file here]`

 follow prerequisite, getting started, then usage section.

 prerequisite:
   requires protobuf-c (for now), and protoc output of yarn_spinner.proto.

 getting started:
    install protobuf-c:
      follow protobuf-c tutorials to use protobuf.

    to create implementation:
      1. requires:
        "yarn_spinner.pb-c.h"

      to be included before you create implementation.

      2. after that, insert this define in "EXACTLY ONE" c/cpp file (translation unit) before including this file.

        #define YARN_C99_IMPLEMENTATION

      to create implementation for this library.
      failing to do so will cause a link error (undefined reference yarn_...).

      if you're willing to go with static function (no dynamic stuff),
      defined this:
        #define YARN_C99_STATIC

      full example below:
        #include "yarn_spinner.pb-c.h" // To create protobuf binding

        #define YARN_C99_IMPLEMENTATION // to activate implementation
        #include "yarn_c99.h" // now you can use yarn c99 functions.


info:
    stubs:
        yarn's line handler / option handler stuff, by default, uses stub operation.
        stab operation just simply spits out the fact that the stub function has been called.

        you can #define YARN_C99_STUB_TO_NOOP to turn these stub function to noop.
*/

#if !defined(YARN_C99_INCLUDE)
#define YARN_C99_INCLUDE

#include <stdint.h>
#include <assert.h>
#include <string.h> /* for strncmp, memset */
#include <stdio.h>  /* TODO: cleanup. basically here for printf debugging */

/*
 * TODO: cleanup
 * TODO: char sign consistency (char can be "signed char" or "unsigned char" by default. ugh)
 *
 * TODO: assert crusade (some error should be reported to game, instead of crashing outright)
 * */

#if !defined(YARN_C99_DEF)
  #if defined(YARN_C99_STATIC)
    #define YARN_C99_DEF static
  #else
    #if defined(__cplusplus)
      #define YARN_C99_DEF extern "C"
    #else
      #define YARN_C99_DEF extern
    #endif
  #endif
#endif

#define YARN_CONCAT1(a, b) a##b
#define YARN_CONCAT(a, b) YARN_CONCAT1(a, b)
#define YARN_LEN(n) (sizeof(n)/(sizeof(0[n])))

#define YARN_KB(n) ((size_t)(n) * 1024)
#define YARN_MB(n) ((size_t)(n) * 1024 * 1024)

#define YARN_STATIC_ASSERT(cond, ident_message) \
    typedef char YARN_CONCAT(yarn_static_assert_line_, YARN_CONCAT(ident_message, __LINE__))[(cond) ? 1 : -1];

#if !defined(yarn_malloc) || !defined(yarn_free) || !defined(yarn_realloc)
  #if !defined(yarn_malloc) && !defined(yarn_free) && !defined(yarn_realloc)
    #include <stdlib.h>
    #define yarn_malloc(alloc_ptr,  sz)      malloc(sz)
    #define yarn_free(alloc_ptr,    ptr)     free(ptr)
    #define yarn_realloc(alloc_ptr, ptr, sz) realloc(ptr, sz)
  #else
    #error "you must define all yarn_{malloc,free,realloc} macro if you're going to define one of them."
  #endif
#endif

struct Yarn__Program;
struct Yarn__Instruction;

typedef struct yarn_dialogue yarn_dialogue;

/*
 * TODO: recipe for fragmentation disaster. fix it
 * */
typedef struct {
    char *id;           /* TODO: @interning can be interned. */
    char *text;
    char *file;         /* TODO: @interning can be interned. */
    char *node;         /* TODO: @interning can be interned. */
    int   line_number;
} yarn_parsed_entry;

typedef struct {
    char *id;

    int  n_substitutions;
    char **substitutions; /* TODO: @interning fragmentation disaster */
} yarn_line;

typedef struct {
    yarn_line line;
    int id;
    char *destination_node;
    int is_available; /* 1 = true, 0 = false */
} yarn_option;

#define YARN_DYN_ARRAY(ty) struct { \
    void *allocator; \
    size_t used; \
    size_t capacity; \
    ty *entries; \
}

typedef YARN_DYN_ARRAY(yarn_parsed_entry) yarn_string_repo;
typedef YARN_DYN_ARRAY(yarn_option)       yarn_option_set;

typedef void yarn_line_handler_func(yarn_dialogue *dialogue, yarn_line *line);
typedef void yarn_option_handler_func(yarn_dialogue *dialogue, yarn_option *options, int options_count);
typedef void yarn_command_handler_func(yarn_dialogue *dialogue, char *command);
typedef void yarn_node_start_handler_func(yarn_dialogue *dialogue, char *node_name);
typedef void yarn_node_complete_handler_func(yarn_dialogue *dialogue, char *node_name);
typedef void yarn_dialogue_complete_handler_func(yarn_dialogue *dialogue);
typedef void yarn_prepare_for_lines_handler_func(yarn_dialogue *dialogue, char **ids, int ids_count);

typedef struct {
    yarn_line_handler_func              *line_handler;
    yarn_option_handler_func            *option_handler;
    yarn_command_handler_func           *command_handler;
    yarn_node_start_handler_func        *node_start_handler;
    yarn_node_complete_handler_func     *node_complete_handler;
    yarn_dialogue_complete_handler_func *dialogue_complete_handler;
    yarn_prepare_for_lines_handler_func *prepare_for_lines_handler;
} yarn_delegates;

/*
    TODO:
    reset this part each time you implement delegate.
*/
YARN_STATIC_ASSERT(sizeof(yarn_delegates) == (sizeof(void *) * 7), delegate_size_mismatch);

/*
 * Stubs!
 */
YARN_C99_DEF yarn_line_handler_func              yarn__stub_line_handler;
YARN_C99_DEF yarn_option_handler_func            yarn__stub_option_handler;
YARN_C99_DEF yarn_command_handler_func           yarn__stub_command_handler;
YARN_C99_DEF yarn_node_start_handler_func        yarn__stub_node_start_handler;
YARN_C99_DEF yarn_node_complete_handler_func     yarn__stub_node_complete_handler;
YARN_C99_DEF yarn_dialogue_complete_handler_func yarn__stub_dialogue_complete_handler;
YARN_C99_DEF yarn_prepare_for_lines_handler_func yarn__stub_prepare_for_lines_handler;

typedef enum {
    YARN_VALUE_NONE, /* corresponds to null, although it's not valid in 2.0 */
    YARN_VALUE_STRING,
    YARN_VALUE_BOOL,
    YARN_VALUE_FLOAT,
} yarn_value_type;

typedef struct {
    yarn_value_type type;
    union {
        char *v_string;
        int   v_bool;
        float v_float;
    } values;
} yarn_value;

typedef yarn_value (*yarn_lib_func)(yarn_dialogue *dialogue);

typedef struct {
    char            *name;
    yarn_lib_func   function;
    int             param_count;
} yarn_functions;

/* TODO:
 * this works under the assumption that variable never collides with each other. */
typedef struct {
    uint32_t   hash;
    size_t     original_str_length;
    yarn_value value;
} yarn_variable_entry;

typedef YARN_DYN_ARRAY(yarn_variable_entry) yarn_variables;
typedef YARN_DYN_ARRAY(yarn_functions)      yarn_library;

typedef enum {
    YARN_EXEC_STOPPED = 0,
    YARN_EXEC_WAITING_OPTION_SELECTION,
    YARN_EXEC_WAITING_FOR_CONTINUE,
    YARN_EXEC_DELIVERING_CONTENT,
    YARN_EXEC_RUNNING,
} yarn_exec_state;

/* TODO: same as lua. */
#define YARN_STACK_CAPACITY 256

struct yarn_dialogue {
    // ProtobufCAllocator *program_allocator;
    void *alloc_ptr; /* will be passed into malloc, free, realloc. */

    Yarn__Program      *program;
    yarn_string_repo    strings;
    yarn_exec_state     execution_state;
    yarn_delegates      delegates;

    yarn_variables  variables;   /* TODO: should it be a hash map? */
    yarn_library    library;     /* TODO: should it be a hash map? */

    yarn_option_set current_options;

    yarn_value stack[YARN_STACK_CAPACITY];

    int stack_ptr;
    int current_node;
    int current_instruction;
};

/* value related helpers. */
/* makes value. */
YARN_C99_DEF yarn_value yarn_none();
YARN_C99_DEF yarn_value yarn_float(float real);
YARN_C99_DEF yarn_value yarn_int(int integer);
YARN_C99_DEF yarn_value yarn_bool(int boolean);
YARN_C99_DEF yarn_value yarn_string(char *string);

/* unravels value. */
YARN_C99_DEF char *yarn_value_as_string(yarn_value value);
YARN_C99_DEF int   yarn_value_as_int(yarn_value value);
YARN_C99_DEF int   yarn_value_as_bool(yarn_value value);
YARN_C99_DEF float yarn_value_as_float(yarn_value value);

/* manipulates Yarn VM. */
YARN_C99_DEF yarn_value yarn_pop_value(yarn_dialogue *dialogue);
YARN_C99_DEF void       yarn_push_value(yarn_dialogue *dialogue, yarn_value value);

YARN_C99_DEF yarn_value yarn_load_variable(yarn_dialogue *dialogue, char *var_name);
YARN_C99_DEF void       yarn_store_variable(yarn_dialogue *dialogue, char *var_name, yarn_value value);

YARN_C99_DEF int   yarn_set_node(yarn_dialogue *dialogue, char *node_name);
YARN_C99_DEF int   yarn_select_option(yarn_dialogue *dialogue, int select_option);
YARN_C99_DEF char *yarn_load_line(yarn_dialogue *dialogue, yarn_line *line);

/* spins Yarn VM. */
YARN_C99_DEF int yarn_is_active(yarn_dialogue *dialogue);
YARN_C99_DEF int yarn_continue(yarn_dialogue *dialogue);

/* Creation / Destroy functions. */
YARN_C99_DEF yarn_dialogue *yarn_create_dialogue_heap();
YARN_C99_DEF void      yarn_destroy_dialogue(yarn_dialogue *dialogue);

/* Function related stuff. */
YARN_C99_DEF yarn_functions *yarn_get_function_with_name(yarn_dialogue *dialogue, char *funcname);
YARN_C99_DEF int yarn_load_functions(yarn_dialogue *dialogue, yarn_functions *functions);

/* Loading functions. */
YARN_C99_DEF int yarn_load_program(yarn_dialogue *dialogue, void *program_buffer, size_t program_length, void *string_table_buffer, size_t string_table_length);

/*
 * Internal functions.
 * (although you can easily call them however you want to,
 *  these function makes a lot of assumption under the hood.
 *  use it wisely, only when you know what you're doing.)
 */

/* Hashes string. */
YARN_C99_DEF uint32_t yarn__hashstr(char *str, size_t strlength);

/*
 * Standard libraries.
 * based on c#'s standard libraries.
 */
YARN_C99_DEF yarn_functions yarn__standard_libs[];

/* tries to extend dynamic array. */
YARN_C99_DEF int yarn__maybe_extend_dyn_array(void **ptr, size_t elem_size, size_t used, size_t *caps, void *allocator);

/*
 * allocates line from current active string repo.
 * essentially pushing line into an array and returning empty string line.
 *
 * TODO: @interning -- this function will be converted into interning string
 * once I'm done with basics.
 */
YARN_C99_DEF yarn_parsed_entry *yarn__alloc_line(yarn_string_repo *string_repo, void *alloc_ptr);

/*
 * allocates new string with substituted value for {0}, {1}, {2}... format.
 * TODO: this could be completely broken. look through.
 */
YARN_C99_DEF char *yarn__substitute_string(char *format, char **substs, int n_substs, void *alloc_ptr);

/*
 * parses csv and loads up into string repo.
 * works under the assumption that you (are loading/loaded) the corresponding program.
 * frees already loaded string table if it exists.
 */
YARN_C99_DEF int yarn__load_string_table(yarn_dialogue *dialogue, void *string_table_buffer, size_t string_table_length);

/*
 * runs single instruction.
 * works under the assumption that you already have working program loaded,
 * and vm is in the running state.
 */
YARN_C99_DEF void yarn__run_instruction(yarn_dialogue *dialogue, Yarn__Instruction *inst);

/*
 * find instruction index based on label.
 * returns -1 if not found.
 */
YARN_C99_DEF int yarn__find_instruction_point_for_label(yarn_dialogue *dialogue, char *label);

/*
 * Resets the state of the dialogue.
 */
YARN_C99_DEF void yarn__reset_state(yarn_dialogue *dialogue);

/*
 * get how many times the node has been visited.
 */
YARN_C99_DEF int  yarn__get_visited_count(yarn_dialogue *dialogue, char *name);

/*
 * uses static buffer to create node name. no free needed.
 * NOTE: it works under the assumption that node name never exceeds more than 1024 character
 */
YARN_C99_DEF char *yarn__get_visited_name_for_node(char *name);

/*
 * Asserts if the yarn dialogue can be continued.
 */
YARN_C99_DEF void yarn__check_if_i_can_continue(yarn_dialogue *dialogue);
#endif

/*
 * ==================================================================
 * Implementations.
 * ==================================================================
 * */

#if defined(YARN_C99_IMPLEMENTATION) && !defined(YARN_C99_INPLEMENT_COMPLETE)
#define YARN_C99_INPLEMENT_COMPLETE

/*
 * Dynamic array stuff.
 */

#define YARN_MAKE_DYNARRAY(target, elemsize, cap, alloc_ctx) do { \
    (target)->allocator = alloc_ctx;                   \
    (target)->capacity  = cap;                         \
    (target)->used      = 0;                           \
    (target)->entries   = yarn_malloc(allocator, cap * elemsize); \
} while(0)

#define YARN_FREE_DYNARRAY(target) do { \
    YARN_STATIC_ASSERT(sizeof(target) == (void *), pass_pointer_to_first_arg_instead_of_value);    \
    yarn_free((target)->allocator, (target)->entries); \
} while(0)

#define YARN_DYNARR_APPEND(v, ent) do {                                           \
  YARN_STATIC_ASSERT(sizeof((v)->entries[0]) == sizeof(ent), type_size_mismatch);              \
  yarn__maybe_extend_dyn_array((void**)&((v)->entries), sizeof(ent),         \
                               ((v)->used), &((v)->capacity), ((v)->allocator)); \
  ((v)->entries)[((v)->used)++] = (ent);                                       \
} while (0)

/* Pops element, and assign it into "var". returns 0 if length == 0. */
#define YARN_DYNARR_POP(v, var) (((v)->used > 0) ? ((var) = ((v)->entries)[--((v)->used)], 1) : 0)


yarn_functions *yarn_get_function_with_name(yarn_dialogue *dialogue, char *funcname) {
    size_t length = strlen(funcname);

    for (int i = 0; i < dialogue->library.used; ++i) {
        yarn_functions *f = &dialogue->library.entries[i];
        if (strlen(f->name) != length) continue;

        if (strncmp(f->name, funcname, length) == 0) {
            return f;
        }
    }

    return 0;
}

 /* simple helper for zero initialized value --
  * return (yarn_value){ 0 } is broken */
yarn_value yarn_none() {
    yarn_value v = { 0 };
    return v;
}

yarn_value yarn_float(float real) {
    yarn_value v = { 0 };
    v.type = YARN_VALUE_FLOAT;
    v.values.v_float = real;
    return v;
}

yarn_value yarn_int(int integer) {
    return yarn_float((float)integer);
}

yarn_value yarn_bool(int boolean) {
    yarn_value v = { 0 };
    v.type = YARN_VALUE_BOOL;
    v.values.v_bool = !!boolean;
    return v;
}

yarn_value yarn_string(char *string) {
    yarn_value v = { 0 };
    v.type = YARN_VALUE_STRING;
    v.values.v_string = string;
    return v;
}

char *yarn_value_as_string(yarn_value value) {
    if (value.type == YARN_VALUE_STRING) {
        return value.values.v_string;
    }

    assert(0 && "Value is not a string!");
    return 0;
}

float yarn_value_as_float(yarn_value value) {
    if (value.type == YARN_VALUE_FLOAT) {
        return value.values.v_float;
    }

    assert(0 && "Value is not a float!");
    return 0;
}

int yarn_value_as_int(yarn_value value) {
    float v = yarn_value_as_float(value);
    /* converting to an int rounds towards 0 (e.g. 10.5 will be 10, 9.9 will be 9) */
    float fraction = v - (int)v;

    /* NOTE: comparing float to 0.0 has defined behavior. it's safe */
    assert(fraction == 0.0 && "Given value is float (has fraction), not an int");
    return (int)v;
}

int yarn_value_as_bool(yarn_value value) {
    if (value.type == YARN_VALUE_BOOL) {
        return !!value.values.v_bool;
    }

    assert(0 && "Value is not a bool!");
    return 0;
}

yarn_value yarn_pop_value(yarn_dialogue *dialogue) {
    assert(dialogue->stack_ptr > 0);
    yarn_value v = dialogue->stack[--dialogue->stack_ptr];
    return v;
}

void yarn_push_value(yarn_dialogue *dialogue, yarn_value value) {
    assert(dialogue->stack_ptr < YARN_LEN(dialogue->stack));
    dialogue->stack[dialogue->stack_ptr++] = value;
}

yarn_value yarn_load_variable(yarn_dialogue *dialogue, char *var_name) {
    size_t length = strlen(var_name);
    uint32_t hash = yarn__hashstr(var_name, length);
    for (int i = 0; i < dialogue->variables.used; ++i) {
        yarn_variable_entry e = dialogue->variables.entries[i];

        if (e.hash == hash && e.original_str_length == length) {
            /* TODO: Hope that it's not colliding and return variable */
            return e.value;
        }
    }

    return yarn_none();
}

void yarn_store_variable(yarn_dialogue *dialogue, char *var_name, yarn_value value) {
    size_t length = strlen(var_name);
    uint32_t hash = yarn__hashstr(var_name, length);

    for (int i = 0; i < dialogue->variables.used; ++i) {
        yarn_variable_entry *e = &dialogue->variables.entries[i];

        if (e->hash == hash && e->original_str_length == length) {
            e->value = value;
            return;
        }
    }

    /* variable does not exist: create new entry instead. */
    yarn_variable_entry entry = {0};
    entry.hash = hash;
    entry.original_str_length = length;
    entry.value = value;

    YARN_DYNARR_APPEND(&dialogue->variables, entry);
}

int yarn_continue(yarn_dialogue *dialogue) {
    yarn__check_if_i_can_continue(dialogue);

    if (dialogue->execution_state == YARN_EXEC_RUNNING) {
        /* cannot continue already running VM. */
        return 0;
    }

    if (dialogue->execution_state == YARN_EXEC_DELIVERING_CONTENT) {
        /* Client called continue after delivering content.
         * set execution_state back, and bail out --
         * it's likely that we're inside delegate. */
        dialogue->execution_state = YARN_EXEC_RUNNING;
        return 0;
    }

    dialogue->execution_state = YARN_EXEC_RUNNING;

    while(dialogue->execution_state == YARN_EXEC_RUNNING) {
        Yarn__Node *node = dialogue->program->nodes[dialogue->current_node]->value;
        Yarn__Instruction *instr = node->instructions[dialogue->current_instruction];

        yarn__run_instruction(dialogue, instr);
        dialogue->current_instruction++;

        if (dialogue->current_instruction > node->n_instructions) {
            dialogue->delegates.node_complete_handler(dialogue, node->name);
            dialogue->execution_state = YARN_EXEC_STOPPED;
            yarn__reset_state(dialogue); /* original version has a setter that resets VM state when operation stops. */

            dialogue->delegates.dialogue_complete_handler(dialogue);
            /* TODO: log message */
        }
    }

    return 0;
}

char *yarn_load_line(yarn_dialogue *dialogue, yarn_line *line) {
    size_t given_id_length = strlen(line->id);
    for (int i = 0; i < dialogue->strings.used; ++i) {
        yarn_parsed_entry *entry = &dialogue->strings.entries[i];
        /* maybe ID points to same memory location? */
        if (entry->id == line->id) return entry->text;

        if (strlen(entry->id) != given_id_length) continue;
        if (strncmp(entry->id, line->id, given_id_length) == 0) {
            return entry->text;
        }
    }
    return 0;
}

int yarn_is_active(yarn_dialogue *dialogue) {
    return dialogue->execution_state != YARN_EXEC_STOPPED;
}

int yarn_set_node(yarn_dialogue *dialogue, char *node_name) {
    assert(dialogue->program && dialogue->program->n_nodes > 0);
    size_t length = strlen(node_name);
    int index = -1;

    Yarn__Node *node = 0;
    for (int i = 0; i < dialogue->program->n_nodes; ++i) {
        if (strncmp(dialogue->program->nodes[i]->key, node_name, length) == 0) {
            index = i;
            node = dialogue->program->nodes[i]->value;
            break;
        }
    }

    if (index == -1) {
        printf("No node named %s has been loaded.\n", node_name);
        assert(0);
    }

    yarn__reset_state(dialogue);
    dialogue->current_node = index;

    if (dialogue->delegates.node_start_handler) {
        dialogue->delegates.node_start_handler(dialogue, node_name);
        if (dialogue->delegates.prepare_for_lines_handler) {
            char **ids = yarn_malloc(dialogue->alloc_ptr, sizeof(void *) * node->n_instructions);
            int n_ids = 0;
            for (int i = 0; i < node->n_instructions; ++i) {
                Yarn__Instruction *inst = node->instructions[i];
                if(inst->opcode == YARN__INSTRUCTION__OP_CODE__RUN_LINE ||
                   inst->opcode == YARN__INSTRUCTION__OP_CODE__ADD_OPTION)
                {
                    ids[n_ids++] = inst->operands[0]->string_value;
                }

            }
            dialogue->delegates.prepare_for_lines_handler(dialogue, ids, n_ids);
            yarn_free(dialogue->alloc_ptr, ids);
        }
    }

    return dialogue->current_node;
}

int yarn_select_option(yarn_dialogue *dialogue, int select_option) {
    if (dialogue->execution_state != YARN_EXEC_WAITING_OPTION_SELECTION) {
        return 0;
    }

    if (dialogue->current_options.used < (size_t)select_option || select_option < 0) {
        return 0;
    }

    yarn_option selected = dialogue->current_options.entries[select_option];
    yarn_push_value(dialogue, yarn_string(selected.destination_node));

    for (int i = 0; i < dialogue->current_options.used; ++i) {
        yarn_option opt = dialogue->current_options.entries[i];
        if (opt.line.n_substitutions > 0) {
            yarn_free(dialogue->alloc_ptr, opt.line.substitutions);
        }
    }

    dialogue->execution_state = YARN_EXEC_WAITING_FOR_CONTINUE;
    dialogue->current_options.used = 0;
    return 1;
}

yarn_dialogue *yarn_create_dialogue_heap(void *allocation_context) {
    yarn_dialogue *dialogue = yarn_malloc(allocation_context, sizeof(yarn_dialogue));
    dialogue->program       = 0;
    dialogue->alloc_ptr     = allocation_context;


    dialogue->strings.used     = 0;
    dialogue->strings.capacity = 512;
    dialogue->strings.entries    = yarn_malloc(dialogue->alloc_ptr, sizeof(yarn_parsed_entry) * 512);

    /* NOTE: it's fine as long as I don't put float or something into yarn_parsed_entry. */
    memset(dialogue->strings.entries, 0, sizeof(yarn_parsed_entry) * 512);

    dialogue->execution_state = YARN_EXEC_STOPPED;
    dialogue->delegates.line_handler              = &yarn__stub_line_handler;
    dialogue->delegates.option_handler            = &yarn__stub_option_handler;
    dialogue->delegates.command_handler           = &yarn__stub_command_handler;
    dialogue->delegates.node_start_handler        = &yarn__stub_node_start_handler;
    dialogue->delegates.node_complete_handler     = &yarn__stub_node_complete_handler;
    dialogue->delegates.dialogue_complete_handler = &yarn__stub_dialogue_complete_handler;
    dialogue->delegates.prepare_for_lines_handler = &yarn__stub_prepare_for_lines_handler;

    dialogue->stack_ptr = 0;
    memset(dialogue->stack, 0, sizeof(yarn_value) * YARN_STACK_CAPACITY);

    dialogue->current_node = 0;
    dialogue->current_instruction = 0;

    YARN_MAKE_DYNARRAY(&dialogue->library, sizeof(yarn_functions), 32, dialogue->alloc_ptr);
    YARN_MAKE_DYNARRAY(&dialogue->variables, sizeof(yarn_variable_entry), 32, dialogue->alloc_ptr);
    YARN_MAKE_DYNARRAY(&dialogue->current_options, sizeof(yarn_option), 32, dialogue->alloc_ptr);

    yarn_load_functions(dialogue, yarn__standard_libs);
    return dialogue;
}

void yarn_destroy_dialogue(yarn_dialogue *dialogue) {
    if (dialogue->program)
        yarn__program__free_unpacked(dialogue->program, 0); /* TODO: @allocator */

    for(size_t i = 0; i < dialogue->strings.used; ++i) {
        yarn_free(dialogue->alloc_ptr, dialogue->strings.entries[i].id);
        yarn_free(dialogue->alloc_ptr, dialogue->strings.entries[i].text);
        yarn_free(dialogue->alloc_ptr, dialogue->strings.entries[i].file);
        yarn_free(dialogue->alloc_ptr, dialogue->strings.entries[i].node);
    }

    for (size_t i = 0; i < dialogue->current_options.used; ++i) {
        if (dialogue->current_options.entries[i].line.n_substitutions > 0) {
            yarn_free(dialogue->alloc_ptr, dialogue->current_options.entries[i].line.substitutions);
        }
    }

    yarn_free(dialogue->alloc_ptr, dialogue->current_options.entries);
    yarn_free(dialogue->alloc_ptr, dialogue->variables.entries);
    yarn_free(dialogue->alloc_ptr, dialogue->library.entries);
    yarn_free(dialogue->alloc_ptr, dialogue->strings.entries);
    yarn_free(dialogue->alloc_ptr, dialogue);
}

int yarn_load_functions(yarn_dialogue *dialogue, yarn_functions *loading_functions) {
    assert(loading_functions);
    int inserted = 0;

    for(int i = 0;; ++i) {
        yarn_functions f = loading_functions[i];
        if (!f.name || !f.function) {
            /* I hit the sentinel! */
            break;
        }

        /*
         * check if other function with the same name has been registered yet.
         * fail if true.
         * TODO: @interning -- function name should be able to intern
         */

        int should_insert = 1;
        size_t new_func_name_len = strlen(f.name);

        for (int x = 0; x < dialogue->library.used; ++x) {
            yarn_functions *entry = &dialogue->library.entries[x];
            size_t entry_name_len = strlen(entry->name);

            if (entry_name_len != new_func_name_len) continue;
            if (strncmp(f.name, entry->name, entry_name_len) != 0) continue;

            printf("Multiple declaration of same name.\n   conflicted name: %s\n", f.name);
            assert(0 && "Multiple declaration of same name");
        }

        if (should_insert) {
            YARN_DYNARR_APPEND(&dialogue->library, f);
            inserted++;
        }
    }

    return inserted;
}

int yarn_load_program(
    yarn_dialogue *dialogue,
    void *program_buffer,
    size_t program_length,
    void *string_table_buffer,
    size_t string_table_length)
{
    if(dialogue->program != 0) {
        yarn__program__free_unpacked(dialogue->program, 0); /* TODO: @allocator */
    }

    dialogue->program = yarn__program__unpack(
        0, /* TODO: @allocator */
        program_length,
        program_buffer);

    int r = yarn__load_string_table(dialogue, string_table_buffer, string_table_length);
    assert(r > 0);
    return 1;
}


/*
 * ====================================================
 * Internals
 * ====================================================
 * */

/* for when I'm parsing stuff */
typedef YARN_DYN_ARRAY(char)  yarn__str_builder;

/* ===========================================
 * Stub delegates.
 */

void yarn__stub_line_handler(yarn_dialogue *dialogue, yarn_line *line) {
#if !defined(YARN_C99_STUB_TO_NOOP)
    printf("Stub: line handler called\n");
    printf("  line ID: %s\n", line->id);
    printf("  line Subst count: %d\n", line->n_substitutions);
#endif

    yarn_continue(dialogue);
}

void yarn__stub_option_handler(yarn_dialogue *dialogue, yarn_option *options, int options_count) {
#if !defined(YARN_C99_STUB_TO_NOOP)
    printf("Stub: option handler called\n");
    printf("  options count: %d\n", options_count);
#endif

    yarn_select_option(dialogue, 0);
}

void yarn__stub_command_handler(yarn_dialogue *dialogue, char *command) {
#if !defined(YARN_C99_STUB_TO_NOOP)
    printf("Stub: command handler called\n");
    printf("  command: %s\n", command);
#endif

    yarn_continue(dialogue);
}

void yarn__stub_node_start_handler(yarn_dialogue *dialogue, char *node_name) {
#if !defined(YARN_C99_STUB_TO_NOOP)
    printf("Stub: node start handler called\n");
    printf("    : beginning node \"%s\"\n", node_name);
#endif
}

void yarn__stub_node_complete_handler(yarn_dialogue *dialogue, char *node_name) {
#if !defined(YARN_C99_STUB_TO_NOOP)
    printf("Stub: node complete handler called\n");
    printf("    : ending node \"%s\"\n", node_name);
#endif
}

void yarn__stub_dialogue_complete_handler(yarn_dialogue *dialogue) {
#if !defined(YARN_C99_STUB_TO_NOOP)
    printf("Stub: dialogue complete handler called\n");
#endif
}

void yarn__stub_prepare_for_lines_handler(yarn_dialogue *dialogue, char **ids, int ids_count) {
#if !defined(YARN_C99_STUB_TO_NOOP)
    printf("Stub: prepare for line handler called\n");
    for (int i = 0; i < ids_count; ++i) {
        printf(" prepare line id %d: %s\n", i, ids[i]);
    }
    if (ids_count == 0) {
        printf("Prepare line was 0\n");
    }
#endif
}

/* ===========================================
 * Utilities.
 */
uint32_t yarn__hashstr(char *str, size_t length) {
    (void)sizeof(length); /* UNUSED */

    /* DJB2 */
    uint32_t h = 5381;
    char c = str[0];
    while((c = *str++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

int yarn__maybe_extend_dyn_array(void **ptr, size_t elem_size, size_t used, size_t *caps, void *alloc_ptr) {
    if (used >= *caps) {
        size_t new_caps = (*caps) * 2;
        void *new_ptr = yarn_realloc(alloc_ptr, *ptr, elem_size * new_caps);

        if (new_ptr) {
            *ptr = new_ptr;
            *caps = new_caps;
        } else {
            /* TODO: logging */
        }

        return 1;
    }
    return 0;
}

/* ===========================================
 * Standard libraries.
 */

yarn_value yarn__number_multiply(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    return yarn_float(left * right);
}

yarn_value yarn__number_divide(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    assert(left != 0.0 && "Division by zero!!!!!!!!");
    return yarn_float(left / right);
}

yarn_value yarn__number_add(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    return yarn_float(left + right);
}

yarn_value yarn__number_sub(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    return yarn_float(left - right);
}

yarn_value yarn__number_modulo(yarn_dialogue *dialogue) {
    int right = yarn_value_as_int(yarn_pop_value(dialogue));
    int left  = yarn_value_as_int(yarn_pop_value(dialogue));
    return yarn_int(left % right);
}

yarn_value yarn__number_unary_minus(yarn_dialogue *dialogue) {
    int right = yarn_value_as_int(yarn_pop_value(dialogue));
    return yarn_int(-right);
}

#define YARN__FLOAT_EPSILON 0.00001
yarn_value yarn__number_eq(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    float e = (right > left ? right : left) - (right > left ? left : right);
    e = (e < 0.0 ? -e : e);
    return yarn_bool(e <= YARN__FLOAT_EPSILON);
}

yarn_value yarn__number_not_eq(yarn_dialogue *dialogue) {
    int eq = yarn_value_as_bool(yarn__number_eq(dialogue));
    return yarn_int(!eq);
}

yarn_value yarn__number_gt(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    return yarn_bool(left > right);
}

yarn_value yarn__number_gte(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    return yarn_bool(left >= right);
}

yarn_value yarn__number_lt(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    return yarn_bool(left < right);
}

yarn_value yarn__number_lte(yarn_dialogue *dialogue) {
    float right = yarn_value_as_float(yarn_pop_value(dialogue));
    float left  = yarn_value_as_float(yarn_pop_value(dialogue));

    return yarn_bool(left <= right);
}

yarn_value yarn__bool_eq(yarn_dialogue *dialogue) {
    int right = yarn_value_as_bool(yarn_pop_value(dialogue));
    int left  = yarn_value_as_bool(yarn_pop_value(dialogue));
    return yarn_bool(left == right);
}

yarn_value yarn__visited(yarn_dialogue *dialogue) {
    char *node_name = yarn_value_as_string(yarn_pop_value(dialogue));
    return yarn_bool(yarn__get_visited_count(dialogue, node_name) > 0);
}

yarn_value yarn__visited_count(yarn_dialogue *dialogue) {
    char *node_name = yarn_value_as_string(yarn_pop_value(dialogue));
    return yarn_int(yarn__get_visited_count(dialogue, node_name));
}

yarn_value yarn__bool_not_eq(yarn_dialogue *dialogue) {
    int right = yarn_value_as_bool(yarn_pop_value(dialogue));
    int left  = yarn_value_as_bool(yarn_pop_value(dialogue));

    return yarn_bool(right != left);
}

yarn_value yarn__bool_and(yarn_dialogue *dialogue) {
    int right = yarn_value_as_bool(yarn_pop_value(dialogue));
    int left  = yarn_value_as_bool(yarn_pop_value(dialogue));

    return yarn_bool(right && left);
}

yarn_value yarn__bool_or(yarn_dialogue *dialogue) {
    int right = yarn_value_as_bool(yarn_pop_value(dialogue));
    int left  = yarn_value_as_bool(yarn_pop_value(dialogue));

    return yarn_bool(right || left);
}

yarn_value yarn__bool_xor(yarn_dialogue *dialogue) {
    int right = yarn_value_as_bool(yarn_pop_value(dialogue));
    int left  = yarn_value_as_bool(yarn_pop_value(dialogue));

    return yarn_bool(right ^ left);
}

yarn_value yarn__bool_not(yarn_dialogue *dialogue) {
    int neg = yarn_value_as_bool(yarn_pop_value(dialogue));
    return yarn_bool(!neg);
}

yarn_functions yarn__standard_libs[] = {
    /* Numbers */
    { "Number.Add",      yarn__number_add,      2 },
    { "Number.Minus",    yarn__number_sub,      2 },
    { "Number.Divide",   yarn__number_divide,   2 },
    { "Number.Multiply", yarn__number_multiply, 2 },
    { "Number.Modulo",   yarn__number_modulo,   2 },

    { "Number.UnaryMinus", yarn__number_unary_minus, 1 },

    { "Number.EqualTo",              yarn__number_eq,       2 },
    { "Number.NotEqualTo",           yarn__number_not_eq,   2 },
    { "Number.GreaterThan",          yarn__number_gt,       2 },
    { "Number.GreaterThanOrEqualTo", yarn__number_gte,      2 },
    { "Number.LessThan",             yarn__number_lt,       2 },
    { "Number.LessThanOrEqualTo",    yarn__number_lte,      2 },

    /* Booleans */
    { "Bool.Not",        yarn__bool_not,        1 },
    { "Bool.EqualTo",    yarn__bool_eq,         2 },
    { "Bool.NotEqualTo", yarn__bool_not_eq,     2 },
    { "Bool.And",        yarn__bool_and,        2 },
    { "Bool.Or",         yarn__bool_or,         2 },
    { "Bool.Xor",        yarn__bool_xor,        2 },

    /* Visited stuff */
    { "visited",         yarn__visited,         1 },
    { "visited_count",   yarn__visited_count,   1 },
    { 0, 0, 0 } /* Sentinel */
};

/* ===========================================
 * Yarn VM functions.
 */

void yarn__check_if_i_can_continue(yarn_dialogue *dialogue) {
    assert(dialogue->program);
    assert(dialogue->program->n_nodes > dialogue->current_node && dialogue->current_node >= 0);
    assert(dialogue->program->nodes[dialogue->current_node]->value);
    assert(dialogue->program->nodes[dialogue->current_node]->value->n_instructions > dialogue->current_instruction && dialogue->current_instruction >= 0);

    assert(dialogue->delegates.line_handler);
    assert(dialogue->delegates.option_handler);
    assert(dialogue->delegates.command_handler);
    assert(dialogue->delegates.node_start_handler);
    assert(dialogue->delegates.node_complete_handler);
    assert(dialogue->delegates.dialogue_complete_handler);
    assert(dialogue->delegates.prepare_for_lines_handler);
}

char *yarn__get_visited_name_for_node(char *name) {
    static char yarn__internal_node_name[8][1024] = {0};
    static int  yarn__internal_node_name_buffer_index = 0;

    if (yarn__internal_node_name_buffer_index == 7) yarn__internal_node_name_buffer_index = 0;
    else yarn__internal_node_name_buffer_index += 1;
    char *buffer = yarn__internal_node_name[yarn__internal_node_name_buffer_index];

    memset(buffer, 0, sizeof(yarn__internal_node_name[0]));

    /* to ensure that user will not accidentally access variable,
     * use character that's guaranteed to be caught as operator by compiler. */
    snprintf(buffer, sizeof(yarn__internal_node_name[0]) - 1,
             ":+INTERNAL+: visited: %s", name);
    return buffer;
}

int yarn__get_visited_count(yarn_dialogue *dialogue, char *name) {
    char *internal_node_name = yarn__get_visited_name_for_node(name);

    yarn_value v = yarn_load_variable(dialogue, internal_node_name);
    if (v.type == YARN_VALUE_NONE) {
        yarn_store_variable(dialogue, internal_node_name, yarn_int(0));
        return 0;
    }

    return yarn_value_as_int(v);
}

void yarn__reset_state(yarn_dialogue *dialogue) {
    dialogue->stack_ptr = 0;
    dialogue->current_instruction = 0;
    dialogue->current_node = 0;
}

int yarn__find_instruction_point_for_label(yarn_dialogue *dialogue, char *label) {
    Yarn__Node *node = dialogue->program->nodes[dialogue->current_node]->value;
    size_t strlength = strlen(label);
    for (int i = 0; i < node->n_labels; ++i) {
        Yarn__Node__LabelsEntry *entry = node->labels[i];
        if (strncmp(entry->key, label, strlength) == 0) {
            return entry->value;
        }
    }

    return -1;
}

void yarn__run_instruction(yarn_dialogue *dialogue, Yarn__Instruction *inst) {
    switch(inst->opcode) {
        case YARN__INSTRUCTION__OP_CODE__STORE_VARIABLE:
        {
            assert(inst->n_operands >= 1);
            yarn_value v = dialogue->stack[dialogue->stack_ptr - 1];
            char *varname = inst->operands[0]->string_value;

            yarn_store_variable(dialogue, varname, v);
        } break;

        case YARN__INSTRUCTION__OP_CODE__PUSH_VARIABLE:
        {
            assert(inst->n_operands >= 1);
            char *varname = inst->operands[0]->string_value;

            yarn_value v = yarn_load_variable(dialogue, varname);
            if (v.type == YARN_VALUE_NONE) {
                size_t varname_length = strlen(varname);
                uint32_t varname_hash = yarn__hashstr(varname, varname_length);
                for (int i = 0; i < dialogue->program->n_initial_values; ++i) {
                    Yarn__Program__InitialValuesEntry *iv = dialogue->program->initial_values[i];
                    size_t str_length = strlen(iv->key);
                    uint32_t hash = yarn__hashstr(iv->key, str_length);

                    if(varname_hash == hash && str_length == varname_length) {
                        if (strncmp(varname, iv->key, varname_length) == 0) {
                            switch(iv->value->value_case) {
                                case YARN__OPERAND__VALUE_STRING_VALUE:
                                    v = yarn_string(iv->value->string_value);
                                    break;

                                case YARN__OPERAND__VALUE_BOOL_VALUE:
                                    v = yarn_bool(iv->value->bool_value);
                                    break;

                                case YARN__OPERAND__VALUE_FLOAT_VALUE:
                                    v = yarn_float(iv->value->float_value);
                                    break;

                                default:
                                    break;
                            }
                        }
                    }
                }
            }

            if (v.type == YARN_VALUE_NONE) {
                printf("Variable %s undefined.\n", varname);
                assert(0);
            } else {
                yarn_push_value(dialogue, v);
            }
        } break;

        case YARN__INSTRUCTION__OP_CODE__STOP:
        {
            Yarn__Node *node = dialogue->program->nodes[dialogue->current_node]->value;
            dialogue->delegates.node_complete_handler(dialogue, node->name);
            dialogue->delegates.dialogue_complete_handler(dialogue);

            /* Increments visited value. */
            int visited = yarn__get_visited_count(dialogue, node->name) + 1;
            yarn_store_variable(dialogue, yarn__get_visited_name_for_node(node->name), yarn_int(visited));

            dialogue->execution_state = YARN_EXEC_STOPPED;
        } break;

        case YARN__INSTRUCTION__OP_CODE__RUN_NODE:
        {
            yarn_value value = yarn_pop_value(dialogue);
            char *node_name = yarn_value_as_string(value);

            Yarn__Node *previous_node = dialogue->program->nodes[dialogue->current_node]->value;
            dialogue->delegates.node_complete_handler(dialogue, previous_node->name);
            /* Increments visited value. */
            int visited = yarn__get_visited_count(dialogue, previous_node->name) + 1;
            yarn_store_variable(dialogue, yarn__get_visited_name_for_node(previous_node->name), yarn_int(visited));

            yarn_set_node(dialogue, node_name);
            dialogue->current_instruction -= 1;
        } break;

        case YARN__INSTRUCTION__OP_CODE__POP:
        {
            yarn_pop_value(dialogue);
        } break;

        case YARN__INSTRUCTION__OP_CODE__PUSH_NULL:
        {
            assert(0 && "Yarn Push NULL opcode is deprecated.");
        } break;

        case YARN__INSTRUCTION__OP_CODE__RUN_COMMAND:
        {
            assert(inst->n_operands >= 1);
            char *command_text = inst->operands[0]->string_value;
            int n_substitutions = 0;

            if (inst->n_operands > 1) {
                /* TODO: same as implementation in C#. */
                int expr_count = (int)inst->operands[1]->float_value;

                /* NOTE: have to check if expr_count is not 0,
                 * otherwise tries to do malloc(0) therefore UB */
                if (expr_count > 0) {
                    /* TODO: @malloc stack allocator would work wonderfully here */
                    char **substitutions = yarn_malloc(dialogue->alloc_ptr, sizeof(char *) * expr_count);
                    n_substitutions = expr_count;

                    for (int i = expr_count - 1; i >= 0; --i) {
                        yarn_value value = yarn_pop_value(dialogue);
                        char *subst = yarn_value_as_string(value);
                        substitutions[i] = subst;
                    }

                    command_text = yarn__substitute_string(command_text, substitutions, n_substitutions, dialogue->alloc_ptr);
                }
            }

            dialogue->execution_state = YARN_EXEC_DELIVERING_CONTENT;
            dialogue->delegates.command_handler(dialogue, command_text);

            if (dialogue->execution_state == YARN_EXEC_DELIVERING_CONTENT) {
                dialogue->execution_state = YARN_EXEC_WAITING_FOR_CONTINUE;
            }

            if (n_substitutions > 0) yarn_free(dialogue->alloc_ptr, command_text);
        } break;

        case YARN__INSTRUCTION__OP_CODE__JUMP:
        {
            assert(dialogue->stack[dialogue->stack_ptr - 1].type == YARN_VALUE_STRING);

            char *label = dialogue->stack[dialogue->stack_ptr - 1].values.v_string;
            dialogue->current_instruction = yarn__find_instruction_point_for_label(dialogue, label) - 1;
        } break;

        case YARN__INSTRUCTION__OP_CODE__JUMP_IF_FALSE:
        {
            int b = yarn_value_as_bool(dialogue->stack[dialogue->stack_ptr - 1]);
            if (!b) {
                assert(inst->n_operands >= 1);
                char *label = inst->operands[0]->string_value;
                dialogue->current_instruction = yarn__find_instruction_point_for_label(dialogue, label) - 1;
            }
        } break;

        case YARN__INSTRUCTION__OP_CODE__JUMP_TO:
        {
            assert(inst->n_operands >= 1);
            char *label = inst->operands[0]->string_value;
            dialogue->current_instruction = yarn__find_instruction_point_for_label(dialogue, label) - 1;
        } break;

        case YARN__INSTRUCTION__OP_CODE__ADD_OPTION:
        {
            assert(inst->n_operands >= 2);

            yarn_option option = { 0 };
            option.line.id = inst->operands[0]->string_value;
            option.destination_node = inst->operands[1]->string_value;

            if (inst->n_operands > 2) {
                /* TODO: same as implementation in C#. */
                int expr_count = (int)inst->operands[2]->float_value;

                /* NOTE: have to check if expr_count is not 0,
                 * otherwise tries to do malloc(0) therefore UB */
                if (expr_count > 0) {
                    /* TODO: @malloc stack allocator would work wonderfully here */
                    option.line.substitutions = yarn_malloc(dialogue->alloc_ptr, sizeof(char *) * expr_count);
                    option.line.n_substitutions = expr_count;

                    for (int i = expr_count - 1; i >= 0; --i) {
                        yarn_value value = yarn_pop_value(dialogue);
                        char *subst = yarn_value_as_string(value);
                        option.line.substitutions[i] = subst;
                    }
                }
            }
            option.is_available = 1; /* defaults to available */

            if (inst->n_operands > 3) {
                int has_line_condition = inst->operands[3]->bool_value;
                if (has_line_condition) {
                    option.is_available = yarn_value_as_bool(yarn_pop_value(dialogue));
                }
            }
            option.id = (int)dialogue->current_options.used;
            YARN_DYNARR_APPEND(&dialogue->current_options, option);
        } break;

        case YARN__INSTRUCTION__OP_CODE__SHOW_OPTIONS:
        {
            if (dialogue->current_options.used == 0) {
                dialogue->execution_state = YARN_EXEC_STOPPED;
                yarn__reset_state(dialogue);
                dialogue->delegates.dialogue_complete_handler(dialogue);
                break;
            }

            /* TODO: C# implementation copies the content of current option.
             *  I wonder why? */

            dialogue->execution_state = YARN_EXEC_WAITING_OPTION_SELECTION;
            dialogue->delegates.option_handler(dialogue, dialogue->current_options.entries, (int)dialogue->current_options.used);

            /* TODO: @Leak when to free this thing? */
            if (dialogue->execution_state == YARN_EXEC_WAITING_FOR_CONTINUE) {
                dialogue->execution_state = YARN_EXEC_RUNNING;
            }
        } break;

        case YARN__INSTRUCTION__OP_CODE__RUN_LINE:
        {
            yarn_line line = { 0 };
            assert(inst->n_operands > 0);

            Yarn__Operand *key_operand = inst->operands[0];
            assert(key_operand->value_case == YARN__OPERAND__VALUE_STRING_VALUE);

            line.id = key_operand->string_value;

            if (inst->n_operands > 1) {
                /* TODO: same as implementation in C#. */
                int expr_count = (int)inst->operands[1]->float_value;

                /* NOTE: have to check if expr_count is not 0,
                 * otherwise tries to do malloc(0) therefore UB */

                if (expr_count > 0) {
                    /* TODO: @malloc stack allocator would work wonderfully here */
                    line.substitutions = yarn_malloc(dialogue->alloc_ptr, sizeof(char *) * expr_count);
                    line.n_substitutions = expr_count;

                    for (int i = expr_count - 1; i >= 0; --i) {
                        yarn_value value = yarn_pop_value(dialogue);
                        char *subst = yarn_value_as_string(value);
                        line.substitutions[i] = subst;
                    }
                }
            }

            dialogue->execution_state = YARN_EXEC_DELIVERING_CONTENT;
            dialogue->delegates.line_handler(dialogue, &line);

            if (dialogue->execution_state == YARN_EXEC_DELIVERING_CONTENT) {
                dialogue->execution_state = YARN_EXEC_WAITING_FOR_CONTINUE;
            }

            if (line.substitutions) yarn_free(dialogue->alloc_ptr, line.substitutions);
        } break;

        case YARN__INSTRUCTION__OP_CODE__PUSH_STRING:
        {
            yarn_value v = { 0 };
            v.type            = YARN_VALUE_STRING;
            v.values.v_string = inst->operands[0]->string_value;
            yarn_push_value(dialogue, v);
        } break;

        case YARN__INSTRUCTION__OP_CODE__PUSH_BOOL:
        {
            yarn_value v = { 0 };
            v.type          = YARN_VALUE_BOOL;
            v.values.v_bool = !!inst->operands[0]->bool_value;
            yarn_push_value(dialogue, v);
        } break;

        case YARN__INSTRUCTION__OP_CODE__PUSH_FLOAT:
        {
            yarn_value v = { 0 };
            v.type           = YARN_VALUE_FLOAT;
            v.values.v_float = inst->operands[0]->float_value;
            yarn_push_value(dialogue, v);
        } break;

        case YARN__INSTRUCTION__OP_CODE__CALL_FUNC:
        {
            assert(inst->n_operands >= 1);

            Yarn__Operand *key_operand = inst->operands[0];
            assert(key_operand->value_case == YARN__OPERAND__VALUE_STRING_VALUE);

            int actual_params = yarn_value_as_int(yarn_pop_value(dialogue));
            char *func_name = key_operand->string_value;
            yarn_functions *func = yarn_get_function_with_name(dialogue, func_name);
            if (!func) {
                printf("Function named `%s`(param count %d) does not exist.\n", func_name, actual_params);
                assert(0);
            }
            assert(func->function);

            int expect_params = func->param_count;
            if (expect_params != actual_params) {
                printf("[YARN SPINNER]: Runtime error -- function named %s expects %d arguments, but %d provided.\n",
                        func_name, expect_params, actual_params);
            }
            assert(expect_params == actual_params && "Parameter count mismatch");

            /* NOTE: @deviation
             *  In original implementation of Yarn VM, the compiler creates an array of
             *  value to pass into functions.
             *
             *  I can do the same thing in here (malloc an array of values to pass),
             *  but I decided to deviate slightly from original for no reason,
             *  and instead allows user to push/pop stack value on their own and
             *  get value instead.
             *
             *  to prevent going deeper into stack and popping important data,
             *  I swap stack pointer to amount of parameter passed temporarily.
             *  this way user receives an error (or crashes) when they try to pop more than they're intended.
             */
            int expect_stack_position = dialogue->stack_ptr - expect_params;

            /* TODO: what if user allocates string here and return it?
             * who holds ownership to that pointer ? */
            yarn_value value = func->function(dialogue);
            assert(expect_stack_position == dialogue->stack_ptr);

            if (value.type != YARN_VALUE_NONE) {
                yarn_push_value(dialogue, value);
            }
        } break;

        default: assert(0 && "Unhandled");
    }
}

/* ===========================================
 * Text manipulation / substitutions.
 */

/* NOTE: returns malloc'ed string. */
char *yarn__substitute_string(char *format, char **substs, int n_substs, void *alloc_ptr) {
    size_t base_strlen = strlen(format);
    size_t memory_size = base_strlen + (base_strlen & 16); /* Align to 16 */

    yarn__str_builder string_builder = {0};
    YARN_MAKE_DYNARRAY(&string_builder, sizeof(char), memory_size, alloc_ptr);

    for (size_t i = 0; i < base_strlen; ++i) {
        if (format[i] == '{') {
            int replace_idx = -1;
            i++; /* skip '{' itself */
            while(format[i++] && format[i] != '}') {
                assert(format[i] >= '0' && format[i] <= '9');
                replace_idx = replace_idx * 10 + (format[i] - '0');
            }

            assert(format[i] == '}');
            i++;
            assert(replace_idx < n_substs);
            char *target_subst = substs[replace_idx];
            char n = 0;
            while((n = *target_subst++)) {
                YARN_DYNARR_APPEND(&string_builder, n);
            }
        } else {
            YARN_DYNARR_APPEND(&string_builder, format[i]);
        }
    }

    /* NOTE:
     * weird C / C++ difference standard.
     * character constant evaluates to int in C, but char in C++.
     * */
    char null_terminator = '\0';
    YARN_DYNARR_APPEND(&string_builder, null_terminator);
    return string_builder.entries;
}

/* ===========================================
 * Parsing strings / CSV tables.
 */
yarn_parsed_entry *yarn__alloc_line(yarn_string_repo *strings, void *alloc_ptr) {
    assert(strings->entries != 0);

    yarn__maybe_extend_dyn_array((void **)&strings->entries, sizeof(strings->entries[0]), strings->used, &strings->capacity, alloc_ptr);
    return &strings->entries[strings->used++];
}

typedef struct {
    char *word;
    size_t size;
} yarn__expect_csv_column;

const yarn__expect_csv_column expect_column[] = {
    { "id",         sizeof("id")         - 1 },
    { "text",       sizeof("text")       - 1 },
    { "file",       sizeof("file")       - 1 },
    { "node",       sizeof("node")       - 1 },
    { "lineNumber", sizeof("lineNumber") - 1 },
};

int yarn__load_string_table(yarn_dialogue *dialogue, void *string_table_buffer, size_t string_table_length) {
    if (dialogue->strings.used > 0) {
        /* TODO: fragmentation disaster. */
        for(size_t i = 0; i < sizeof(dialogue->strings.used); ++i) {
            yarn_free(dialogue->alloc_ptr, dialogue->strings.entries[i].id);
            yarn_free(dialogue->alloc_ptr, dialogue->strings.entries[i].text);
            yarn_free(dialogue->alloc_ptr, dialogue->strings.entries[i].file);
            yarn_free(dialogue->alloc_ptr, dialogue->strings.entries[i].node);
        }
    }

    char *begin = string_table_buffer;
    size_t current     = 0;
    size_t advanced    = 0;
    size_t line_count  = 0;
    int column_size    = 0;
    int current_column = 0;

    int parsing_first_line = 1;
    int in_quote = 0;

    yarn_parsed_entry *line = yarn__alloc_line(&dialogue->strings, dialogue->alloc_ptr);

    while(current < string_table_length) {
        assert(current_column <= column_size);

        switch(begin[advanced]) {
            /* We're done!! */
            case '\0':
            {
                current = advanced;
                goto done;
            } break;

            case '"':
            {
                /* 1. csv escapes double quotation with itself apparently. wtf? */
                if (in_quote && begin[advanced+1] == '"') {
                    advanced += 2;
                } else {
                    advanced++;
                    in_quote = !in_quote;
                }
            } break;

            case '\n':
            {
                if (in_quote) {
                    advanced++;
                } else {
                    assert(current_column == column_size);
                    if (!parsing_first_line) {
                        line_count++;
                        line = yarn__alloc_line(&dialogue->strings, dialogue->alloc_ptr);
                    } else {
                        parsing_first_line = 0;
                    }

                    current_column = 0;
                    advanced++; /* skip line break */
                    current = advanced;
                }
            } break;

            case ',':
            {
                if (in_quote) {
                    advanced++;
                } else {

                    if (parsing_first_line) {
                        assert(column_size < YARN_LEN(expect_column));
                        const yarn__expect_csv_column *expect = &expect_column[column_size];

                        /* TODO: handle more gracefully. */
                        size_t consumed_since = advanced - current;
                        if (expect->size != consumed_since) {
                            printf("Expect size difference: %zu to %zu\n", expect->size, consumed_since);
                            return 0;
                        }

                        char *current_ptr = &begin[current];
                        if (strncmp(current_ptr, expect->word, expect->size) != 0) {
                            char *dupped_str = yarn_malloc(dialogue->alloc_ptr, consumed_since + 1);
                            memset(dupped_str, 0, consumed_since);
                            strncpy(dupped_str, current_ptr, consumed_since);

                            printf("Expect word difference: %s to %s\n", expect->word, dupped_str);
                            yarn_free(dialogue->alloc_ptr, dupped_str);
                            return 0;
                        }

                        column_size++;
                    } else {

                        /* NOTE: @CSVParsing
                         * this part of the code must parse line number as an
                         * integer.
                         * */
                        assert(current < advanced);
                        assert(advanced <= string_table_length);

                        size_t consumed_since = 0;

                        if (begin[advanced - 1] == '"') {
                            /* been in quote!! */
                            consumed_since = (advanced - 1) - (current + 1);
                            current += 1;
                        } else {
                            consumed_since = advanced - current;
                        }

                        char *current_ptr = &begin[current];
                        if (current_column == 4) {
                            /* TODO: let's limit line size to 65535 for now. */
                            assert(consumed_since <= 5 && "line number size more than 65535 (todo)");

                            int result_number = 0;
                            for (int i = 0; i < consumed_since; ++i) {
                                /* TODO: handle other than ascii */
                                char c = begin[current+i];
                                int  n = c - '0';
                                assert((n >= 0 && n <= 9) && "number evaluated to something different");
                                /* if I'm going to handle maximum numbers, do this to check overflow;
                                 * int a = INT_MAX - n;
                                 * int b = a / 10;
                                 * assert(result_number <= b);
                                 * */

                                result_number = result_number * 10 + n;
                            }

                            line->line_number = result_number;
                        } else {
                            /* NOTE:
                             * aligns memory to more than 64 */
                            size_t actual_size = consumed_since + 1;
                            ptrdiff_t mod = (actual_size) & (63);
                            actual_size += mod;

                            char *dupped_str = yarn_malloc(dialogue->alloc_ptr, actual_size);

                            memset(dupped_str, 0, actual_size);
                            strncpy(dupped_str, current_ptr, consumed_since);

                            /* super complicated for just removing double quotation though. */
                            char *has_dquote_escaped = 0;
                            while((has_dquote_escaped = strstr(dupped_str, "\"\""))) {
                                size_t following_text = (size_t)(has_dquote_escaped - dupped_str);
                                memmove(has_dquote_escaped + 1, has_dquote_escaped + 2, following_text);
                                dupped_str[consumed_since--] = '\0';
                            }

                            if (current_column == 0) {        /* id */
                                line->id = dupped_str;
                            } else if (current_column == 1) { /* text */
                                line->text = dupped_str;
                            } else if (current_column == 2) { /* file */
                                line->file = dupped_str;
                            } else if (current_column == 3) {
                                line->node = dupped_str;
                            } else {
                                assert(0 && "Unknown column detected.");
                            }
                        }
                    }

                    current_column++;
                    advanced++; /* skip comma */
                    current = advanced;
                }
            } break;

            default: advanced++;
        }
    }

done:

    assert(current == string_table_length);
    assert(in_quote == 0 && "Unexpected eof while parsing double quotation");
    return 1;
}

#endif
