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
      full example below:
        #include <protobuf-c/protobuf-c.h>
        #include "yarn_spinner.pb-c.h" // To create protobuf binding

        #define YARN_C99_IMPLEMENTATION // to activate implementation
        #include "yarn_c99.h" // now you can use yarn c99 functions.

      1. requires:
        "yarn_spinner.pb-c.h"

      to be included before you create implementation.

      2. after that, insert this define in "EXACTLY ONE" c/cpp file (translation unit) before including this file.

        #define YARN_C99_IMPLEMENTATION

      to create implementation for this library.
      failing to do so will cause a link error (undefined reference yarn_...).

info:
    stubs:
        yarn's line handler / option handler stuff, by default, uses stub operation.
        stab operation just simply spits out the fact that the stub function has been called.

        you can #define YARN_C99_STUB_TO_NOOP to turn these stub function to noop.
*/

#if !defined(YARN_C99_INCLUDE)
#define YARN_C99_INCLUDE

#include <stddef.h>
#include <stdint.h>

/*
 * TODO:
 *
 * - cleanup, tidy-up
 * - handle uncertain lifetime problem about "substitutions" stuff.
 *
 * - more consistent memory handling (malloc festival with varying lifetime invites serious disaster)
 *   - (@intern)         some wasteful string allocation should be interned instead.
 *   - (@allocator)      unstable / inconsistent lifetime, cause of fragmentation, not using YARN_MALLOC, etc that can be solved with allocator.
 *   - (@data-structure) performing slow lookups that can be solved with appropriate data structure.
 *
 * - assert crusade (some error should be reported to game, instead of crashing outright)
 *   - (@logging) logging function should report this.
 *
 * - make feeling closer to C# (simulate garbage-collected string with allocator?)
 *   - (@deviation) have to consult to other users, or weird design decisions.
 *   - (@limit)     unnecessary limit that still remains in code because it was easier to do that way. not anymore. fix
 *
 * - more tests (most of the code feels pretty fragile, though this should be done after most of the todo above is done)
 * - compile without protobuf (create converter? this can't be that hard, just receive generated code and reallocate everything into my own program defined here.)
 * */

#if !defined(YARN_C99_DEF)
  #define YARN_C99_DEF extern
#endif

/* Yarn Dialogue:
 * corresponds to YarnSpinner.Dialogue in C# implementation.
 * it is the core of this library.
 * */
typedef struct yarn_dialogue yarn_dialogue;

/* Yarn Value:
 * corresponds to YarnSpinner.Value in C# implementation.
 * */
typedef struct yarn_value yarn_value;

/* Yarn Variable Storage:
 * corresponds to YarnSpinner.IVariableStorage in C# implementation.
 * */
typedef struct yarn_variable_storage yarn_variable_storage;


/* =========================================
 * Interface for variable storage:
 * For Start:
 *   you can make default storage:
 *     yarn_variable_storage storage = yarn_create_default_storage();
 *     yarn_dialogue *dialogue = yarn_create_dialogue(storage);
 *
 *   and do this when it's done
 *     yarn_destroy_dialogue(dialogue);
 *     yarn_destroy_default_storage(storage);
 *
 *   default storage plays the same role as InMemoryVariableStorage in C# implementation.
 *   it uses hashmap underneath.
 *
 * alternatively, you can create your own storage:
 *   1. make functions, like this
 *     struct SomethingStore {};
 *     yarn_value load_func(SomethingStore *store, char *varname) { ... }
 *     void       save_func(SomethingStore *store, char *varname, yarn_value value) { ... }
 *
 *   2. create interface for struct below.
 *
 *     struct SomethingStore *store = make_somethingstore();
 *     yarn_variable_storage istorage = {0};
 *
 *     istorage.data = store;
 *     istorage.load = load_func;
 *     istorage.save = save_func;
 *
 *   3. pass it into dialogue
 *     yarn_dialogue *dialogue = yarn_create_dialogue(istorage);
 *
 */

typedef yarn_value yarn_load_variable_func(void *storage, char *name);
typedef void       yarn_save_variable_func(void *storage, char *name, yarn_value value);

struct yarn_variable_storage {
    void *data;
    yarn_load_variable_func *load;
    yarn_save_variable_func *save;
};

typedef enum {
    YARN_VALUE_NONE = 0, /* corresponds to null, although it's not valid in 2.0 */
    YARN_VALUE_STRING,
    YARN_VALUE_BOOL,
    YARN_VALUE_FLOAT,
} yarn_value_type;

struct yarn_value {
    yarn_value_type type;
    union {
        char *v_string;
        int   v_bool;
        float v_float;
    } values;
};

/* =========================================
 * Hashmap.
 * Unlike dynamic array above, this one is also usable in user space.
 *
 * TODO: IT IS NOT TYPESAFE!!!!!!!!!!!!!!!!!!!:
 *   kvcreate takes type to it's first argument so it SEEMS like it is typesafe,
 *   but it's not. it does not store the information. it's misleading.
 *   it is actually storing the SIZE of the type. more on that below.
 * 
 * overview of how it's implemented:
 *   linear-probing, djb2 hashing.
 *   only supports char* as key, and key will be cloned on push.
 * 
 *   internal data is stored in char* buffer.
 *   like the diagram shown below;
 *
 * |================= entire buffer ==================|
 * [header-data|actual value][header-data|actual value]
 * |===== one "chunk"  =====|
 *
 * with map->element_size being set to the size of actual value.
 * on kvpush, it will take the size of passed value, and check against the map it's being inserted to,
 * and fires assertion error if does not.
 *
 * TODO: @testing -- this works, but complete and thorough test must be done.
 *
 * create hashmap with:
 *  map = yarn_kvcreate(element, capacity);
 *
 *  then, operate with:
 *    yarn_kvpush(&map, "hello", value);
 *    yarn_kvget(&map, "hello", &value); // will write into value
 *    yarn_kvhas(&map, "hello"); // returns 1 on true, 0 on false
 *    yarn_kvdelete(&map, "hello");
 *
 *  once you're done:
 *    yarn_kvdestroy(&map); // every value stored inside will be unreachable.
 */

typedef struct {
    uint32_t hash;
    size_t   keylen;
    char    *key;
} yarn_kvpair_header;

typedef struct {
    char   *entries;
    size_t used;
    size_t capacity;
    size_t element_size; /* size of the actual value it's made for, not the size of chunk */
} yarn_kvmap;

#define yarn_kvcreate(element, caps) \
    yarn__kvmap_create(sizeof(element), (caps))

#define yarn_kvdestroy(map) \
    yarn__kvmap_destroy((map))

#define yarn_kvpush(map, key, value) \
    yarn__kvmap_pushsize((map), (key), &(value), sizeof(value))

#define yarn_kvget(map, key, value_ptr) \
    yarn__kvmap_get((map), (key), (value_ptr), ((value_ptr) ? sizeof(*value_ptr) : 0))

#define yarn_kvhas(map, key) \
    (yarn__kvmap_get((map), (key), 0, 0) != -1)

#define yarn_kvdelete(map, key) \
    yarn__kvmap_delete((map), (key))

/* =============================================
 * quick and dirty type-safe dynamic array
 */

#define YARN_DYN_ARRAY(ty) struct { \
    size_t used; \
    size_t capacity; \
    ty *entries; \
}

/* macro for push/pop/create/destroy is inside implementation block below. */

/* =============================================
 * Yarn line:
 * Same as C# implementation.
 * 
 * TODO: @allocator the lifetime of substitions array cannot be trusted right now.
 * do not copy the pointer, clone the entire array if you want the lifetime to be certain.
 */

typedef struct {
    char *id;

    int  n_substitutions;
    char **substitutions; /* TODO: @allocator fragmentation disaster */
} yarn_line;

/*
 * TODO: @allocator recipe for fragmentation disaster. fix it
 * */
typedef struct {
    char *id;
    char *text;
    char *file;         /* TODO: @intern can be interned. */
    char *node;         /* TODO: @intern can be interned. */
    int   line_number;
} yarn_parsed_entry;

typedef YARN_DYN_ARRAY(yarn_parsed_entry) yarn_string_table;


/* =============================================
 * Yarn options:
 * TODO: @allocator same warning as line applies here.
 */

typedef struct {
    yarn_line line;
    int id;
    char *destination_node;
    int is_available; /* 1 = true, 0 = false */
} yarn_option;

typedef YARN_DYN_ARRAY(yarn_option) yarn_option_set;

/* =============================================
 * Yarn functions:
 * TODO: @deviation match it to C# implementation. SUBJECT TO CHANGE
 *
 * for now, it works similar to how Lua's function works,
 * but much more barebone. you have to know how VM works.
 *
 * define function with signature of yarn_function below,
 *    yarn_value add(yarn_dialogue *dialogue) {
 *        // it's stack based, means you have to pop it backwards
 *
 *        int right = yarn_value_as_int(yarn_pop_value(dialogue));
 *        int left  = yarn_value_as_int(yarn_pop_value(dialogue));
 *        return yarn_int(left + right);
 *    }
 *
 * then create const array with sentinel at the bottom:
 *    yarn_func_reg entries[] = {
 *      { "add", add, 2 }, // name, function pointer, argument count
 *      { 0, 0, 0 } // Sentinel
 *    };
 *
 * then call "yarn_load_functions"
 *    yarn_load_functions(dialogue, entries);
 */
typedef yarn_value yarn_function(yarn_dialogue *dialogue);

typedef struct {
    char            *name;
    yarn_function   *function;
    int              param_count;
} yarn_func_reg;

typedef struct {
    yarn_function   *function;
    int              param_count;
} yarn_function_entry;

typedef yarn_kvmap yarn_library;

/* =============================================
 * Yarn delegates:
 * TODO: @deviation match it to C# implementation. SUBJECT TO CHANGE
 */
typedef void yarn_line_handler_func(yarn_dialogue *dialogue, yarn_line *line);
typedef void yarn_option_handler_func(yarn_dialogue *dialogue, yarn_option *options, int options_count);
typedef void yarn_command_handler_func(yarn_dialogue *dialogue, char *command);
typedef void yarn_node_start_handler_func(yarn_dialogue *dialogue, char *node_name);
typedef void yarn_node_complete_handler_func(yarn_dialogue *dialogue, char *node_name);
typedef void yarn_dialogue_complete_handler_func(yarn_dialogue *dialogue);
typedef void yarn_prepare_for_lines_handler_func(yarn_dialogue *dialogue, char **ids, int ids_count);

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


/* =============================================
 * Yarn Dialogue:
 * A ported runtime from C# implementation.
 */

/* TODO: @deviation C# version allows more than this. */
#define YARN_STACK_CAPACITY 256

/* Protobuf stuff. */
struct Yarn__Program;
struct Yarn__Node;
struct Yarn__Instruction;

typedef enum {
    YARN_EXEC_STOPPED = 0,
    YARN_EXEC_WAITING_OPTION_SELECTION,
    YARN_EXEC_WAITING_FOR_CONTINUE,
    YARN_EXEC_DELIVERING_CONTENT,
    YARN_EXEC_RUNNING,
} yarn_exec_state;


struct yarn_dialogue {
    // ProtobufCAllocator *program_allocator;

    struct Yarn__Program *program;
    yarn_string_table  *strings;
    yarn_exec_state     execution_state;

    /* Delegates. */
    yarn_line_handler_func              *line_handler;
    yarn_option_handler_func            *option_handler;
    yarn_command_handler_func           *command_handler;
    yarn_node_start_handler_func        *node_start_handler;
    yarn_node_complete_handler_func     *node_complete_handler;
    yarn_dialogue_complete_handler_func *dialogue_complete_handler;
    yarn_prepare_for_lines_handler_func *prepare_for_lines_handler;

    yarn_variable_storage storage;
    yarn_library library;

    yarn_option_set current_options;
    yarn_value stack[YARN_STACK_CAPACITY];

    int stack_ptr;
    int current_node;
    int current_instruction;
};

/* ====================================================
 * Function declarations.
 * */

/* Creation / Destroy functions. */
YARN_C99_DEF yarn_dialogue *yarn_create_dialogue(yarn_variable_storage storage);
YARN_C99_DEF void           yarn_destroy_dialogue(yarn_dialogue *dialogue);

YARN_C99_DEF yarn_variable_storage yarn_create_default_storage(void);
YARN_C99_DEF void                  yarn_destroy_default_storage(yarn_variable_storage storage);

YARN_C99_DEF yarn_string_table *yarn_create_string_table(void);
YARN_C99_DEF void               yarn_destroy_string_table(yarn_string_table *table);

/* Loading functions. */
YARN_C99_DEF int yarn_load_program(yarn_dialogue *dialogue, void *program_buffer, size_t program_length);
YARN_C99_DEF int yarn_load_string_table(yarn_string_table *table, void *csv_buffer, size_t csv_length);

/* value related helpers. */
/* makes value. */
YARN_C99_DEF yarn_value yarn_none(void);
YARN_C99_DEF yarn_value yarn_float(float real);
YARN_C99_DEF yarn_value yarn_int(int integer);
YARN_C99_DEF yarn_value yarn_bool(int boolean);
YARN_C99_DEF yarn_value yarn_string(char *string); /* TODO: @allocator I should clone passed string here. */

/* unravels value. */
YARN_C99_DEF char *yarn_value_as_string(yarn_value value);
YARN_C99_DEF int   yarn_value_as_int(yarn_value value);
YARN_C99_DEF int   yarn_value_as_bool(yarn_value value);
YARN_C99_DEF float yarn_value_as_float(yarn_value value);

/* formatting value.
 * must be freed with destroy_formatted_string. */
YARN_C99_DEF char *yarn_format_value(yarn_value value);
YARN_C99_DEF void  yarn_destroy_formatted_string(char *formatted_string);

/* check if yarn vm is currently running. */
YARN_C99_DEF int yarn_is_active(yarn_dialogue *dialogue);

/* spin up VM, until it hits operation that needs to be handled. */
YARN_C99_DEF int yarn_continue(yarn_dialogue *dialogue);


YARN_C99_DEF int yarn_set_node(yarn_dialogue *dialogue, char *node_name); /* sets current node. */
YARN_C99_DEF int yarn_select_option(yarn_dialogue *dialogue, int select_option); /* selects option, if option is active. */

/* stack operations. */
YARN_C99_DEF yarn_value yarn_pop_value(yarn_dialogue *dialogue);
YARN_C99_DEF void       yarn_push_value(yarn_dialogue *dialogue, yarn_value value);

/* variable storage operations.
 * NOTE: it just calls storage.load / storage.value inside.
 * */
YARN_C99_DEF yarn_value yarn_load_variable(yarn_dialogue *dialogue, char *var_name);
YARN_C99_DEF void       yarn_store_variable(yarn_dialogue *dialogue, char *var_name, yarn_value value);

/* loads line, and performs substitution.
 * NOTE: @allocator
 *    1. convert function consumes line. it is unsafe to access inner data after you've called this function.
 *    2. returned value must be freed with yarn_destroy_displayable_line.
 * */
YARN_C99_DEF char *yarn_convert_to_displayable_line(yarn_string_table *table, yarn_line *line);
YARN_C99_DEF void  yarn_destroy_displayable_line(char *line);

/* Function related stuff. */
YARN_C99_DEF yarn_function_entry  yarn_get_function_with_name(yarn_dialogue *dialogue, char *funcname);
YARN_C99_DEF int                  yarn_load_functions(yarn_dialogue *dialogue, yarn_func_reg *functions);

/*
 * ====================================================
 * Internals
 * ====================================================
 * */

/* Standard libraries.
 * based on c#'s standard libraries. definitions will be inside the IMPLEMENTATION block. 
 * TODO: string is not supported yet. */
extern yarn_func_reg yarn__standard_libs[];

/* Hashes string. */
YARN_C99_DEF uint32_t yarn__hashstr(char *str, size_t strlength);

/* strndup implementation that uses YARN_MALLOC. */
YARN_C99_DEF char *yarn__strndup(char *original, size_t length);

/* tries to extend dynamic array. */
YARN_C99_DEF int yarn__maybe_extend_dyn_array(void **ptr, size_t elem_size, size_t used, size_t *caps);

/* hashmap API. */
YARN_C99_DEF yarn_kvmap yarn__kvmap_create(size_t elem_size, size_t caps); /* returns newly created map with given capacity and element size. */
YARN_C99_DEF void yarn__kvmap_destroy(yarn_kvmap *map);                    /* frees entire map. */

/* pushes element into map. element_size must match with map->impl->element_size. */
YARN_C99_DEF int yarn__kvmap_pushsize(yarn_kvmap *map, char *key, void *value, size_t element_size); 

/* get element from map, and write into value if given valid pointer. returns -1 if does not exist. */
YARN_C99_DEF int yarn__kvmap_get(yarn_kvmap *map, char *key, void *value, size_t element_size);

/* delete element from map, and rearrange. */
YARN_C99_DEF int yarn__kvmap_delete(yarn_kvmap *map, char *key);

/* recreate the whole map if the current map's used up space meets certain threshold. */
YARN_C99_DEF int yarn__kvmap_maybe_rehash(yarn_kvmap *map);

/*
 * allocates line from current active string repo.
 * essentially pushing line into an array and returning empty string line.
 */
YARN_C99_DEF yarn_parsed_entry *yarn__alloc_line(yarn_string_table *string_table);

/* allocates new string with substituted value for {0}, {1}, {2}... format. */
YARN_C99_DEF char *yarn__substitute_string(char *format, char **substs, int n_substs);

/* parses csv and loads up into string repo. */
YARN_C99_DEF int yarn__load_string_table(yarn_string_table *table, void *string_table_buffer, size_t string_table_length);

/* runs single instruction. */
YARN_C99_DEF void yarn__run_instruction(yarn_dialogue *dialogue, struct Yarn__Instruction *inst);

/*
 * find instruction index based on label.
 * returns -1 if not found.
 */
YARN_C99_DEF int yarn__find_instruction_point_for_label(yarn_dialogue *dialogue, char *label);

/* Resets the state of the dialogue. */
YARN_C99_DEF void yarn__reset_state(yarn_dialogue *dialogue);

/* get how many times the node has been visited. */
YARN_C99_DEF int  yarn__get_visited_count(yarn_dialogue *dialogue, char *name);

/* uses static buffer to create node name. no free needed.
 * NOTE: it works under the assumption that node name never exceeds more than 1024 character */
YARN_C99_DEF char *yarn__get_visited_name_for_node(char *name);

/* Asserts if the yarn dialogue can be continued. */
YARN_C99_DEF void yarn__check_if_i_can_continue(yarn_dialogue *dialogue);

/* functions for default (dynamic array based) storage. */
YARN_C99_DEF yarn_value yarn__load_from_default_storage(void *istorage, char *var_name);
YARN_C99_DEF void       yarn__save_into_default_storage(void *istorage, char *var_name, yarn_value value);

#endif

/*
 * ==================================================================
 * Implementations.
 * ==================================================================
 * */

#if defined(YARN_C99_IMPLEMENTATION) && !defined(YARN_C99_IMPLEMENT_COMPLETE)
#define YARN_C99_IMPLEMENT_COMPLETE

#include <assert.h>
#include <string.h> /* for strncmp, memset */
#include <stdio.h>  /* TODO: @cleanup cleanup. basically here for printf debugging */

#if !defined(YARN_MALLOC) || !defined(YARN_FREE) || !defined(YARN_REALLOC)
  #if !defined(YARN_MALLOC) && !defined(YARN_FREE) && !defined(YARN_REALLOC)
    #include <stdlib.h>
    #define YARN_MALLOC(sz)       malloc(sz)
    #define YARN_FREE(ptr)        free(ptr)
    #define YARN_REALLOC(ptr, sz) realloc(ptr, sz)
  #else
    #error "you must define all yarn_{malloc,free,realloc} macro if you're going to define one of them."
  #endif
#endif


#define YARN_CONCAT1(a, b) a##b
#define YARN_CONCAT(a, b) YARN_CONCAT1(a, b)
#define YARN_LEN(n) (sizeof(n)/(sizeof(0[n])))

#define YARN_STATIC_ASSERT(cond, ident_message) \
    typedef char YARN_CONCAT(yarn_static_assert_line_, YARN_CONCAT(ident_message, __LINE__))[(cond) ? 1 : -1];

/*
 * Dynamic array stuff.
 */

#define YARN_MAKE_DYNARRAY(target, elemtype, cap) do { \
    (target)->capacity  = cap;                         \
    (target)->used      = 0;                           \
    (target)->entries   = YARN_MALLOC(cap * sizeof(elemtype)); \
} while(0)

/* TODO: @cleanup why is it so different from hashmap???? */
#define YARN_DYNARR_APPEND(v, ent) do {                                             \
  YARN_STATIC_ASSERT(sizeof((v)->entries[0]) == sizeof(ent), type_size_mismatch);   \
  yarn__maybe_extend_dyn_array((void**)&((v)->entries), sizeof(ent),                \
                               ((v)->used), &((v)->capacity));                      \
  ((v)->entries)[((v)->used)++] = (ent);                                            \
} while (0)

/* Pops element, and assign it into "var". returns 0 if length == 0. */
#define YARN_DYNARR_POP(v, var) (((v)->used > 0) ? ((var) = ((v)->entries)[--((v)->used)], 1) : 0)


yarn_function_entry yarn_get_function_with_name(yarn_dialogue *dialogue, char *funcname) {
    size_t length = strlen(funcname);
    uint32_t hash = yarn__hashstr(funcname, length);

    yarn_function_entry entry = {0};
    int exists = yarn_kvget(&dialogue->library, funcname, &entry);

    return entry;
}

 /* simple helper for zero initialized value */
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

char *yarn_format_value(yarn_value value) {
    switch(value.type) {

        case YARN_VALUE_BOOL:
            return (!!value.values.v_bool) ? "true" : "false";

        case YARN_VALUE_STRING:
            return yarn__strndup(value.values.v_string, strlen(value.values.v_string));

        case YARN_VALUE_FLOAT: /* the tricky one... */
        {
            char temp[1024] = {0};
            int result_length = snprintf(temp, sizeof(temp)-1, "%f", value.values.v_float);

            return yarn__strndup(temp, result_length);
        } break;

        case YARN_VALUE_NONE:
        default:
            return "None";
    }
}

void  yarn_destroy_formatted_string(char *formatted_string) {
    size_t length = strlen(formatted_string);

    /* Static text will be ignored. */
    if (length == 4 && strncmp(formatted_string, "true",  length) == 0) return;
    if (length == 5 && strncmp(formatted_string, "false", length) == 0) return;
    if (length == 4 && strncmp(formatted_string, "None",  length) == 0) return;

    YARN_FREE(formatted_string);
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
    return dialogue->storage.load(dialogue->storage.data, var_name);
}

void yarn_store_variable(yarn_dialogue *dialogue, char *var_name, yarn_value value) {
    dialogue->storage.save(dialogue->storage.data, var_name, value);
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
        struct Yarn__Node *node = dialogue->program->nodes[dialogue->current_node]->value;
        struct Yarn__Instruction *instr = node->instructions[dialogue->current_instruction];

        yarn__run_instruction(dialogue, instr);
        dialogue->current_instruction++;

        if (dialogue->current_instruction > node->n_instructions) {
            dialogue->node_complete_handler(dialogue, node->name);
            dialogue->execution_state = YARN_EXEC_STOPPED;
            yarn__reset_state(dialogue); /* original version has a setter that resets VM state when operation stops. */

            dialogue->dialogue_complete_handler(dialogue);
            /* TODO: @logging log message */
        }
    }

    return 0;
}

char *yarn_convert_to_displayable_line(yarn_string_table *table, yarn_line *line) {
    size_t given_id_length = strlen(line->id);
    char *result = 0;

    for (int i = 0; i < table->used; ++i) {
        yarn_parsed_entry *entry = &table->entries[i];
        /* maybe ID points to same memory location? */
        if (entry->id == line->id) return entry->text;

        if (strlen(entry->id) != given_id_length) continue;
        if (strncmp(entry->id, line->id, given_id_length) == 0) {
            result = entry->text;
            break;
        }
    }

    if (result) {
        if (line->n_substitutions > 0) {
            result = yarn__substitute_string(result, line->substitutions, line->n_substitutions);
        } else {
            /* so you can still free it with `yarn_destroy_displayable_line` */
            result = yarn__strndup(result, strlen(result));
        }
    } else {
        /* TODO: @logging */
        printf("line id %s does not exist.\n", line->id);
        printf("NOTE: the line will still be consumed\n");
    }

    /* no matter how actual providing goes, still consume the base line. */
    /* TODO: FIXME: @allocator
     * this is confusing and certainly will be unexpected for users.
     * remove it and find some other way to keep memory from leaking. */
    if (line->n_substitutions > 0) {
        for (int i = 0; i < line->n_substitutions; ++i) {
            yarn_destroy_formatted_string(line->substitutions[i]);
        }
        YARN_FREE(line->substitutions);

        /* Consume substitutions. */
        line->n_substitutions = 0;
        line->substitutions   = 0;
    }
    return result;
}

void yarn_destroy_displayable_line(char *line) {
    YARN_FREE(line);
}

int yarn_is_active(yarn_dialogue *dialogue) {
    return dialogue->execution_state != YARN_EXEC_STOPPED;
}

int yarn_set_node(yarn_dialogue *dialogue, char *node_name) {
    assert(dialogue->program && dialogue->program->n_nodes > 0);
    size_t length = strlen(node_name);
    int index = -1;

    struct Yarn__Node *node = 0;
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

    if (dialogue->node_start_handler) {
        dialogue->node_start_handler(dialogue, node_name);
        if (dialogue->prepare_for_lines_handler) {
            char **ids = YARN_MALLOC(sizeof(void *) * node->n_instructions);
            int n_ids = 0;
            for (int i = 0; i < node->n_instructions; ++i) {
                struct Yarn__Instruction *inst = node->instructions[i];
                if(inst->opcode == YARN__INSTRUCTION__OP_CODE__RUN_LINE ||
                   inst->opcode == YARN__INSTRUCTION__OP_CODE__ADD_OPTION)
                {
                    ids[n_ids++] = inst->operands[0]->string_value;
                }

            }
            dialogue->prepare_for_lines_handler(dialogue, ids, n_ids);
            YARN_FREE(ids);
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
            for (int i = 0; i < opt.line.n_substitutions; ++i) {
                yarn_destroy_formatted_string(opt.line.substitutions[i]);
            }
            YARN_FREE(opt.line.substitutions);

            opt.line.n_substitutions = 0;
            opt.line.substitutions   = 0;
        }
    }

    dialogue->execution_state = YARN_EXEC_WAITING_FOR_CONTINUE;
    dialogue->current_options.used = 0;
    return 1;
}

yarn_dialogue *yarn_create_dialogue(yarn_variable_storage storage) {
    yarn_dialogue *dialogue = YARN_MALLOC(sizeof(yarn_dialogue));

    dialogue->program = 0;
    dialogue->strings = 0;
    dialogue->storage = storage;

    dialogue->current_node        = 0;
    dialogue->current_instruction = 0;
    dialogue->execution_state     = YARN_EXEC_STOPPED;

    dialogue->stack_ptr = 0;
    memset(dialogue->stack, 0, sizeof(yarn_value) * YARN_STACK_CAPACITY);

    dialogue->line_handler              = &yarn__stub_line_handler;
    dialogue->option_handler            = &yarn__stub_option_handler;
    dialogue->command_handler           = &yarn__stub_command_handler;
    dialogue->node_start_handler        = &yarn__stub_node_start_handler;
    dialogue->node_complete_handler     = &yarn__stub_node_complete_handler;
    dialogue->dialogue_complete_handler = &yarn__stub_dialogue_complete_handler;
    dialogue->prepare_for_lines_handler = &yarn__stub_prepare_for_lines_handler;

    dialogue->library = yarn_kvcreate(yarn_function_entry, 32);
    YARN_MAKE_DYNARRAY(&dialogue->current_options, yarn_option, 32);

    yarn_load_functions(dialogue, yarn__standard_libs);
    return dialogue;
}

void yarn_destroy_dialogue(yarn_dialogue *dialogue) {
    if (dialogue->program)
        yarn__program__free_unpacked(dialogue->program, 0); /* TODO: @allocator */

    for (size_t i = 0; i < dialogue->current_options.used; ++i) {
        yarn_option opt = dialogue->current_options.entries[i];
        if (opt.line.n_substitutions > 0) {
            for (int i = 0; i < opt.line.n_substitutions; ++i) {
                yarn_destroy_formatted_string(opt.line.substitutions[i]);
            }
            YARN_FREE(opt.line.substitutions);
        }
    }

    yarn_kvdestroy(&dialogue->library);
    YARN_FREE(dialogue->current_options.entries);
    YARN_FREE(dialogue);
}

yarn_variable_storage yarn_create_default_storage() {

    yarn_kvmap kvmap = yarn_kvcreate(yarn_value, 64);
    yarn_kvmap *m = YARN_MALLOC(sizeof(yarn_kvmap));

    *m = kvmap;

    yarn_variable_storage storage = {0};
    storage.data = m;
    storage.load = &yarn__load_from_default_storage;
    storage.save = &yarn__save_into_default_storage;

    return storage;
}

void yarn_destroy_default_storage(yarn_variable_storage storage) {
    yarn_kvmap *kvmap = storage.data;
    yarn_kvdestroy(kvmap);
    YARN_FREE(storage.data);
}

yarn_string_table *yarn_create_string_table() {
    yarn_string_table *table = YARN_MALLOC(sizeof(yarn_string_table));
    YARN_MAKE_DYNARRAY(table, yarn_parsed_entry, 512);
    return table;
}

void yarn_destroy_string_table(yarn_string_table *table) {
    if (table->used > 0) {
        /* TODO: @allocator fragmentation disaster. */
        for(size_t i = 0; i < table->used; ++i) {
            YARN_FREE(table->entries[i].id);
            YARN_FREE(table->entries[i].text);
            YARN_FREE(table->entries[i].file);
            YARN_FREE(table->entries[i].node);
        }
    }

    YARN_FREE(table->entries);
    YARN_FREE(table);
}

int yarn_load_functions(yarn_dialogue *dialogue, yarn_func_reg *loading_functions) {
    assert(loading_functions);
    int inserted = 0;

    for(int i = 0;; ++i) {
        yarn_func_reg f = loading_functions[i];
        if (!f.name || !f.function) {
            /* I hit the sentinel! */
            break;
        }

        if(yarn_kvhas(&dialogue->library, f.name)) {
            printf("Multiple declaration of same name.\n   conflicted name: %s\n", f.name);
            assert(0 && "Multiple declaration of same name");
        }

        yarn_function_entry ent = {0};
        ent.function    = f.function;
        ent.param_count = f.param_count;
        yarn_kvpush(&dialogue->library, f.name, ent);
    }

    return inserted;
}

int yarn_load_program(
    yarn_dialogue *dialogue,
    void *program_buffer,
    size_t program_length)
{
    if(dialogue->program != 0) {
        yarn__program__free_unpacked(dialogue->program, 0); /* TODO: @allocator */
    }

    dialogue->program = yarn__program__unpack(
        0, /* TODO: @allocator */
        program_length,
        program_buffer);

    return 1;
}

int yarn_load_string_table(yarn_string_table *str_table, void *csv, size_t csv_size) {
    int r = yarn__load_string_table(str_table, csv, csv_size);
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
 * Stub 
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
 * Data structure.
 */

#define YARN__KV_INDEXOF(pmap, idx) (yarn_kvpair_header *)((pmap)->entries + ((sizeof(yarn_kvpair_header) + (pmap)->element_size) * idx))

yarn_kvmap yarn__kvmap_create(size_t elem_size, size_t caps) {
    yarn_kvmap map = {0};

    size_t chunk_size  = sizeof(yarn_kvpair_header) + elem_size;
    map.element_size = elem_size;
    map.capacity     = caps;
    map.entries      = YARN_MALLOC(chunk_size * caps);
    map.used         = 0;
    assert(map.entries);

    for (int i = 0; i < map.capacity; ++i) {
        yarn_kvpair_header *header = YARN__KV_INDEXOF(&map, i);

        header->hash   = 0;
        header->keylen = 0;
        header->key    = 0;
    }

    return map;
}

#define YARN__KVMAP_THRESHOLD 0.7
int yarn__kvmap_maybe_rehash(yarn_kvmap *map) {
    if ((map->capacity * YARN__KVMAP_THRESHOLD) < map->used) {
        yarn_kvmap new_map = yarn__kvmap_create(map->element_size, map->capacity * 2);

        for (int i = 0; i < map->capacity; ++i) {
            yarn_kvpair_header *header = YARN__KV_INDEXOF(map, i);
            if (header->key) {
                void *value = (void *)(header + 1);
                yarn__kvmap_pushsize(&new_map, header->key, value, map->element_size);
                YARN_FREE(header->key);
                header->key = 0;
            }
        }

        YARN_FREE(map->entries);
        *map = new_map;
        return 1;
    }
    return 0;
}

void yarn__kvmap_destroy(yarn_kvmap *map) {
    for (int i = 0; i < map->capacity; ++i) {
        yarn_kvpair_header *header = YARN__KV_INDEXOF(map, i);
        if (header->key) {
            YARN_FREE(header->key);
            header->key = 0;
        }
    }

    YARN_FREE(map->entries);
}

int yarn__kvmap_pushsize(yarn_kvmap *map, char *key, void *value, size_t element_size) {
    assert(map && map->element_size == element_size);
    assert(key && value);
    yarn__kvmap_maybe_rehash(map);

    size_t keylen   = strlen(key);
    uint32_t hash   = yarn__hashstr(key, keylen);
    uint32_t bucket = hash % map->capacity;
    yarn_kvpair_header *header = YARN__KV_INDEXOF(map, bucket);

    while(header->key) {
        if (header->hash == hash && header->keylen == keylen) {
            if (strncmp(header->key, key, keylen) == 0) {
                void *insert_here = (void*)(header + 1);
                memcpy(insert_here, value, element_size);

                return (int)bucket;
            }
        }

        bucket = (bucket + 1) % map->capacity;
        header = YARN__KV_INDEXOF(map, bucket);
    }

    /* New insertion. */
    header->hash   = hash;
    header->keylen = keylen;
    header->key    = yarn__strndup(key, keylen);

    void *insert_here = (void*)(header + 1);
    memcpy(insert_here, value, element_size);

    map->used += 1;
    return (int)bucket;
}

int yarn__kvmap_get(yarn_kvmap *map, char *key, void *value, size_t element_size) {
    assert(map && key && map->entries);
    if (value)
        assert(element_size == map->element_size);
    else
        assert(element_size == 0);

    size_t keylen   = strlen(key);
    uint32_t hash   = yarn__hashstr(key, keylen);
    uint32_t bucket = hash % map->capacity;
    yarn_kvpair_header *header = YARN__KV_INDEXOF(map, bucket);

    while(header->key) {
        if (header->hash == hash && header->keylen == keylen) {
            if (strncmp(header->key, key, keylen) == 0) {
                if (value) {
                    void *exists = (void*)(header+1);
                    memcpy(value, exists, element_size);
                }

                return (int) bucket;
            }
        }

        bucket = (bucket + 1) % map->capacity;
        header = YARN__KV_INDEXOF(map, bucket);
    }

    return -1;
}

int yarn__kvmap_delete(yarn_kvmap *map, char *key) {
    assert(map && key);
    if (map->used == 0) return -1;

    size_t keylen   = strlen(key);
    uint32_t hash   = yarn__hashstr(key, keylen);
    uint32_t bucket = hash % map->capacity;

    yarn_kvpair_header *emptied = 0;
    yarn_kvpair_header *header  = 0;
    while(header->key) {
        if (header->hash == hash && header->keylen == keylen) {
            if (!emptied) { /* hasn't found yet. delete the entry itself. */
                if (strncmp(header->key, key, keylen) == 0) {
                    YARN_FREE(header->key);
                    header->key    = 0;
                    header->keylen = 0;
                    header->hash   = 0;

                    emptied = header;
                }
            } else {
                /* move current header's content to emptied cell, then mark current header as emptied */
                emptied->key    = header->key;
                emptied->keylen = header->keylen;
                emptied->hash   = header->hash;

                header->keylen = 0;
                header->hash   = 0;
                header->key    = 0; /* don't free, emptied->key holds it */

                emptied = header;
                /* continue probing until actually empty cell is found */
            }
        }

        bucket = (bucket + 1) % map->capacity;
        header = YARN__KV_INDEXOF(map, bucket);
    }

    if (emptied) map->used -= 1;

    return (!!emptied); /* return if there's a pointer assigned for once -- meaning if the deletion actually happened or not */
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

int yarn__maybe_extend_dyn_array(void **ptr, size_t elem_size, size_t used, size_t *caps) {
    if (used >= *caps) {
        size_t new_caps = (*caps) * 2;
        void *new_ptr = YARN_REALLOC(*ptr, elem_size * new_caps);

        if (new_ptr) {
            *ptr = new_ptr;
            *caps = new_caps;
        } else {
            /* TODO: @logging */
        }

        return 1;
    }
    return 0;
}

char *yarn__strndup(char *cloning, size_t length) {
    char *result = YARN_MALLOC(length + 1);
    memset(result, 0, length + 1);

    for (size_t i = 0; i < length; ++i) {
        result[i] = cloning[i];
    }

    return result;
}

/* ===========================================
 * Dynamic Array based default storage functions.
 */

yarn_value yarn__load_from_default_storage(void *istorage, char *var_name) {
    yarn_value value = yarn_none();
    int exists = yarn_kvget((yarn_kvmap*)istorage, var_name, &value);

    return value;
}

void yarn__save_into_default_storage(void *istorage, char *var_name, yarn_value value) {
    yarn_kvmap map = { istorage };
    yarn_kvpush((yarn_kvmap*)istorage, var_name, value);
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

yarn_func_reg yarn__standard_libs[] = {
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

    assert(dialogue->line_handler);
    assert(dialogue->option_handler);
    assert(dialogue->command_handler);
    assert(dialogue->node_start_handler);
    assert(dialogue->node_complete_handler);
    assert(dialogue->dialogue_complete_handler);
    assert(dialogue->prepare_for_lines_handler);
}

char *yarn__get_visited_name_for_node(char *name) {
    static char ring[8][1024] = {0};
    static int  ringidx = 0;

    char *buffer = ring[ringidx];

    memset(buffer, 0, sizeof(ring[0]));

    /* to ensure that user will not accidentally access variable,
     * use space/parentheses for visited count
     * (hoping that yarn spinner won't allow them as variable name) */
    snprintf(buffer, sizeof(ring[0]) - 1, "(internal): visited node `%s`", name);

    ringidx = (ringidx + 1) % 8;
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

            /* check if there's an entry in initial_values */
            if (v.type == YARN_VALUE_NONE) {
                size_t varname_length = strlen(varname);
                uint32_t varname_hash = yarn__hashstr(varname, varname_length);
                Yarn__Program__InitialValuesEntry *iv;

                for (int i = 0; i < dialogue->program->n_initial_values; ++i) {
                    iv = dialogue->program->initial_values[i];
                    size_t iv_length = strlen(iv->key);
                    uint32_t iv_hash = yarn__hashstr(iv->key, iv_length);

                    if (iv_hash   != varname_hash)   continue;
                    if (iv_length != varname_length) continue;
                    if (strncmp(varname, iv->key, iv_length) != 0) continue;

                    /* matched! */

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
            dialogue->node_complete_handler(dialogue, node->name);
            dialogue->dialogue_complete_handler(dialogue);

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
            dialogue->node_complete_handler(dialogue, previous_node->name);
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
                int expr_count = (int)inst->operands[1]->float_value;

                /* NOTE: have to check if expr_count is not 0,
                 * otherwise tries to do malloc(0) therefore implementation-defined */
                if (expr_count > 0) {
                    /* TODO: @allocator stack allocator would work wonderfully here */
                    char **substitutions = YARN_MALLOC(sizeof(char *) * expr_count);
                    n_substitutions = expr_count;

                    for (int i = expr_count - 1; i >= 0; --i) {
                        yarn_value value = yarn_pop_value(dialogue);
                        char *subst = yarn_format_value(value);
                        substitutions[i] = subst;
                    }

                    command_text = yarn__substitute_string(command_text, substitutions, n_substitutions);

                    for (int i = 0; i < n_substitutions; ++i) {
                        yarn_destroy_formatted_string(substitutions[i]);
                    }
                    YARN_FREE(substitutions);
                }
            }

            dialogue->execution_state = YARN_EXEC_DELIVERING_CONTENT;
            dialogue->command_handler(dialogue, command_text);

            if (dialogue->execution_state == YARN_EXEC_DELIVERING_CONTENT) {
                dialogue->execution_state = YARN_EXEC_WAITING_FOR_CONTINUE;
            }

            if (n_substitutions > 0) YARN_FREE(command_text);
        } break;

        case YARN__INSTRUCTION__OP_CODE__JUMP:
        {
            yarn_value *jump_to = &dialogue->stack[dialogue->stack_ptr - 1];
            assert(jump_to->type == YARN_VALUE_STRING);

            char *label = jump_to->values.v_string;
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
                int expr_count = (int)inst->operands[2]->float_value;

                /* NOTE: have to check if expr_count is not 0,
                 * otherwise tries to do malloc(0) therefore implementation defined */
                if (expr_count > 0) {
                    /* TODO: @allocator stack allocator would work wonderfully here */
                    option.line.substitutions = YARN_MALLOC(sizeof(char *) * expr_count);
                    option.line.n_substitutions = expr_count;

                    for (int i = expr_count - 1; i >= 0; --i) {
                        yarn_value value = yarn_pop_value(dialogue);
                        char *subst = yarn_format_value(value);
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
                dialogue->dialogue_complete_handler(dialogue);
                break;
            }

            /* TODO: @deviation C# implementation copies the content of current option.
             *  I wonder why? */

            dialogue->execution_state = YARN_EXEC_WAITING_OPTION_SELECTION;
            dialogue->option_handler(dialogue, dialogue->current_options.entries, (int)dialogue->current_options.used);

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
                int expr_count = (int)inst->operands[1]->float_value;

                /* NOTE: have to check if expr_count is not 0,
                 * otherwise tries to do malloc(0) therefore implementation defined */

                if (expr_count > 0) {
                    /* TODO: @allocator stack allocator would work wonderfully here */
                    line.substitutions = YARN_MALLOC(sizeof(char *) * expr_count);
                    line.n_substitutions = expr_count;

                    for (int i = expr_count - 1; i >= 0; --i) {
                        yarn_value value = yarn_pop_value(dialogue);
                        char *subst = yarn_format_value(value);
                        line.substitutions[i] = subst;
                    }
                }
            }

            dialogue->execution_state = YARN_EXEC_DELIVERING_CONTENT;
            dialogue->line_handler(dialogue, &line);

            if (dialogue->execution_state == YARN_EXEC_DELIVERING_CONTENT) {
                dialogue->execution_state = YARN_EXEC_WAITING_FOR_CONTINUE;
            }

            /* line substitution is left without consumed. free now. */
            if (line.n_substitutions > 0) {
                for (int i = 0; i < line.n_substitutions; ++i) {
                    YARN_FREE(line.substitutions[i]);
                }
                YARN_FREE(line.substitutions);

                /* Consume substitutions. */
                line.n_substitutions = 0;
                line.substitutions   = 0;
            }
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
            yarn_function_entry func = yarn_get_function_with_name(dialogue, func_name);
            if (!func.function) {
                printf("Function named `%s`(param count %d) does not exist.\n", func_name, actual_params);
                assert(0);
            }

            int expect_params = func.param_count;
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
             */
            int expect_stack_position = dialogue->stack_ptr - expect_params;

            /* TODO: @allocator what if user allocates string here and return it?
             * who holds ownership to that pointer ? */
            yarn_value value = func.function(dialogue);
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
char *yarn__substitute_string(char *format, char **substs, int n_substs) {
    assert(format && substs);

    size_t base_strlen = strlen(format);
    size_t allocate_size = base_strlen * 2;

    yarn__str_builder string_builder = {0};
    YARN_MAKE_DYNARRAY(&string_builder, char, allocate_size);

    for (size_t i = 0; i < base_strlen; ++i) {
        if (format[i] == '{') {
            i += 1;

            int replace_idx = 0;
            char c = 0;
            while((c = format[i]) && c != '}') {
                assert(c >= '0' && c <= '9');
                replace_idx = replace_idx * 10 + (c - '0');
                i++;
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
yarn_parsed_entry *yarn__alloc_line(yarn_string_table *strings) {
    assert(strings->entries != 0);

    yarn__maybe_extend_dyn_array((void **)&strings->entries, sizeof(strings->entries[0]), strings->used, &strings->capacity);
    return &strings->entries[strings->used++];
}

typedef struct {
    char *word;
    size_t size;
} yarn__expect_csv_column;

/* TODO: @limit this shouldn't be concretely defined like this. it's not guaranteed that the csv's column would look exactly like this */
const yarn__expect_csv_column expect_column[] = {
    { "id",         sizeof("id")         - 1 },
    { "text",       sizeof("text")       - 1 },
    { "file",       sizeof("file")       - 1 },
    { "node",       sizeof("node")       - 1 },
    { "lineNumber", sizeof("lineNumber") - 1 },
};

/* quick and dirty CSV parsing. */
int yarn__load_string_table(yarn_string_table *table, void *string_table_buffer, size_t string_table_length) {
    char *begin = string_table_buffer;
    size_t current     = 0;
    size_t advanced    = 0;
    int column_count   = 0;
    int current_column = 0;

    int parsing_first_line = 1;
    int in_quote = 0;

    yarn_parsed_entry *line = yarn__alloc_line(table);

    while(current < string_table_length) {
        assert(current_column <= column_count);

        switch(begin[advanced]) {
            /* We're done!! */
            case '\0':
            {
                current = advanced;
                goto done;
            } break;

            case '"':
            {
                /* 1. csv escapes double quotation with itself apparently. what the crap? */
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
                    assert(current_column == column_count);
                    if (!parsing_first_line) {
                        line = yarn__alloc_line(table);
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
                        assert(column_count < YARN_LEN(expect_column));
                        const yarn__expect_csv_column *expect = &expect_column[column_count];

                        /* TODO: @logging handle more gracefully. */
                        size_t consumed_since = advanced - current;
                        if (expect->size != consumed_since) {
                            printf("Expect size difference: %zu to %zu\n", expect->size, consumed_since);
                            return 0;
                        }

                        char *current_ptr = &begin[current];
                        if (strncmp(current_ptr, expect->word, expect->size) != 0) {
                            char *dupped_str = yarn__strndup(current_ptr, consumed_since);

                            printf("Expect word difference: %s to %s\n", expect->word, dupped_str);
                            YARN_FREE(dupped_str);
                            return 0;
                        }

                        column_count++;
                    } else {

                        /* TODO: @deviation
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
                            /* TODO: @limit let's limit line size to 65535 for now. */
                            assert(consumed_since <= 5 && "line number size more than 65535 (todo)");

                            int result_number = 0;
                            for (int i = 0; i < consumed_since; ++i) {
                                /* TODO: @limit handle other than ascii */
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
                            char *dupped_str = yarn__strndup(current_ptr, consumed_since);

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
