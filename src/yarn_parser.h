/* TODO: requires yarn_c99 */

#if !defined(YARN_PARSER_INCLUDE) && defined(YARN_C99_IMPLEMENT_COMPLETE)
#define YARN_PARSER_INCLUDE

typedef struct yarn_program yarn_program;
struct yarn_program {
    int success;
};

typedef struct {
    char  *memory;
    char  *filename;

    size_t length;
} yarn_source;

typedef struct {
    yarn_allocator allocator;
} yarn_compiler;

typedef struct {
    int success;
} yarn_program;

typedef YARN_DYN_ARRAY(yarn_source) yarn_source_array;

typedef struct yarn_compilation_job yarn_compilation_job;
struct yarn_compilation_job {
    yarn_source_array sources;
};

YARN_C99_DEF yarn_compilation_job *yarn_create_compilation_job();
YARN_C99_DEF void                  yarn_destroy_compilation_job(yarn_compilation_job *job);

YARN_C99_DEF int yarn_add_script_mem(yarn_compilation_job *job, const char *filename, const char *memory, size_t length);
YARN_C99_DEF int yarn_add_script(yarn_compilation_job *job, const char *filename);

YARN_C99_DEF yarn_program *yarn_compile(yarn_compilation_job *job);

YARN_C99_DEF int yarn__add_script(yarn_compilation_job *job, const char *filename, const char *memory, size_t length);
#endif

#if defined(YARN_PARSER_IMPLEMENTATION) && defined(YARN_C99_IMPLEMENT_COMPLETE) && !defined(YARN_PARSER_IMPLEMENT_COMPLETE)
#define YARN_PARSER_IMPLEMENT_COMPLETE

enum {
    YARN_TOK_INVALID = 0,

    YARN_TOK_ESCAPE,  /*  \ */
    YARN_TOK_COMMENT, /* // */
    YARN_TOK_NEWLINE,

    /* Basic data literals. */
    YARN_TOK_IDENTIFIER,
    YARN_TOK_NUMBER,
    YARN_TOK_STRING,

    YARN_TOK_NODE_BEGIN, /* --- */
    YARN_TOK_NODE_END,   /* === */

    YARN_COMMAND_BEGIN,  /* << */
    YARN_COMMAND_END,    /* >> */

    YARN_TOK_OPTION_ARROW, /* -> */

    YARN_TOK_HASHTAG,    /* # */

    /* Predefined types. */
    YARN_TOK_TYPE_BOOL,
    YARN_TOK_TYPE_NUMBER,
    YARN_TOK_TYPE_STRING,

    /* Reserved commands. */
    YARN_TOK_SET,
    YARN_TOK_JUMP,
    YARN_TOK_TO,
    YARN_TOK_DECLARE,

    /* control flow keywords */
    YARN_TOK_IF,
    YARN_TOK_ELSEIF,
    YARN_TOK_ELSE,
    YARN_TOK_ENDIF,

    /* Control flow stuff */
    YARN_TOK_INDENT,
    YARN_TOK_DEDENT,

    /* misc keywords */
    YARN_TOK_AS,

    /* Expressions */
    YARN_TOK_COLON,
    YARN_TOK_SEMICOLON,

    YARN_TOK_DOLLAR,
    YARN_TOK_OPENPAREN,
    YARN_TOK_CLOSEPAREN,
    YARN_TOK_HYPHEN,
    YARN_TOK_BANG,
    YARN_TOK_ASTERISK,
    YARN_TOK_SLASH,
    YARN_TOK_PERCENT,
    YARN_TOK_PLUS,
    YARN_TOK_OPENBRACK,
    YARN_TOK_CLOSEBRACK,
    YARN_TOK_MORE,
    YARN_TOK_LESS,
    YARN_TOK_LESSEQ,
    YARN_TOK_MOREEQ,
    YARN_TOK_EQEQ,
    YARN_TOK_EQUAL,
    YARN_TOK_NOTEQ,
    YARN_TOK_AND,
    YARN_TOK_OR,
    YARN_TOK_XOR,

    YARN_TOK_COUNT,
};

yarn_compilation_job *yarn_create_compilation_job() {
    yarn_compilation_job *c = YARN_MALLOC(sizeof(yarn_compilation_job));
    YARN_MAKE_DYNARRAY(&c->sources, yarn_source, 16);
    return c;
}

void yarn_destroy_compilation_job(yarn_compilation_job *job) {
    for (int i = 0; i < job->sources.used; ++i) {
        YARN_FREE(job->sources.entries[i].memory);
        YARN_FREE(job->sources.entries[i].filename);
    }

    YARN_FREE(job->sources.entries);
    YARN_FREE(job);
}

int yarn_add_script_mem(yarn_compilation_job *job, const char *filename, const char *memory, size_t length) {
    for (int i = 0; i < job->sources.used; ++i) {
        if (strcmp(job->sources.entries[i].filename, filename) == 0) {
            printf("(yarn_add_script) warning: file `%s` already exists in compilation job. ignoring\n", filename);
            return 0;
        }
    }

    yarn_source source = {0};
    source.filename = yarn__strndup(filename, strlen(filename));
    source.memory   = yarn__strndup(memory,   length);
    source.length   = length;

    YARN_DYNARR_APPEND(&job->sources, source);

    return 1;
}

int yarn_add_script(yarn_compilation_job *job, const char *filename) {
    assert(filename && "Compiling error on yarn_add_script: filename is empty!");
    for (int i = 0; i < job->sources.used; ++i) {
        if (strcmp(job->sources.entries[i].filename, filename) == 0) {
            printf("(yarn_add_script) warning: file `%s` already exists in compilation job. ignoring\n", filename);
            return 0;
        }
    }

    FILE *fp = NULL;
    fp = fopen(filename, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        size_t length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (length > 0) {
            char *buffer = (char *)YARN_MALLOC(length + 1);
            assert(buffer && "Compiling error on yarn_add_script: failed to allocate buffer!");

            memset(buffer, 0, length + 1);
            size_t read = fread(buffer, 1, length, fp);
            YARN_UNUSED(read); /* TODO: @Unused */
            char *filename_copied = yarn__strndup(filename, strlen(filename));

            yarn_source source = {0};
            source.filename = filename_copied;
            source.memory   = buffer;
            source.length   = length;
            YARN_DYNARR_APPEND(&job->sources, source);

            fclose(fp);
            return 1;
        } else {
            printf("(yarn_add_script) error: given file `%s` has zero length (empty file)\n", filename);
        }

        fclose(fp);
    } else {
        printf("(yarn_add_script) error: failed to read file `%s`\n", filename);
    }
    return 0;
}


yarn_program yarn_compile(yarn_compilation_job *job) {
    yarn_compiler compiler = {0};
    yarn_program program = {0};
    compiler.allocator = yarn_create_allocator(8 * 1024);

    yarn_destroy_allocator(compiler.allocator);
    return program;
}

#endif
