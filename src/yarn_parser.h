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

typedef YARN_DYN_ARRAY(yarn_source) yarn_source_array;

typedef struct yarn_compilation_job yarn_compilation_job;
struct yarn_compilation_job {
    yarn_source_array sources;
};

YARN_C99_DEF yarn_compilation_job *yarn_create_compilation_job();
YARN_C99_DEF void                  yarn_destroy_compilation_job(yarn_compilation_job *job);

YARN_C99_DEF int yarn_add_script_mem(yarn_compilation_job *job, const char *filename, const char *memory, size_t length);
YARN_C99_DEF int yarn_add_script(yarn_compilation_job *job, const char *filename);

YARN_C99_DEF yarn_program yarn_compile(yarn_compilation_job *job);

YARN_C99_DEF int yarn__add_script(yarn_compilation_job *job, const char *filename, const char *memory, size_t length);
#endif

#if defined(YARN_PARSER_IMPLEMENTATION) && defined(YARN_C99_IMPLEMENT_COMPLETE) && !defined(YARN_PARSER_IMPLEMENT_COMPLETE)
#define YARN_PARSER_IMPLEMENT_COMPLETE

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

/* =================================================
 * Internals.
 * ================================================= */

enum {
    YARN_LEXMODE_HEADER     = 0,
    YARN_LEXMODE_BODY_TEXT  = 1,
    YARN_LEXMODE_EXPRESSION = 2,
    YARN_LEXMODE_COMMENT    = 3,
    YARN_LEXMODE_STRING     = 4,
};

typedef struct {
    int mode; /* Current lex mode */

    int cursor;
    int prev_cursor;
    int line;
    int grapheme_column; /* For UTF-8. */

    char   *string;      /* Could be an ident */
    int    str_length;
    float  number;
    int    boolean;

    yarn_source source;
} yarn__lexer;

static inline int yarn__isdigit(char c) {
    if ('0' <= c && c <= '9') {
        return 1;
    } else {
        return 0;
    }
}

static void yarn__lexprep(yarn__lexer *l) {
    l->prev_cursor = l->cursor;

    l->string     = 0;
    l->str_length = 0;
    l->number     = 0;
    l->boolean    = 0;
}

static void yarn__lexclear(yarn__lexer *l) {
    l->cursor = 0;
    l->prev_cursor = 0;
    l->line = 1;
    l->grapheme_column = 0;

    yarn__lexprep(l);
}

static char yarn__lexadvance(yarn__lexer *l) {
    if (l->cursor >= l->source.length) return 0;
    if (l->cursor < 0)                 return 0;

    return l->source.memory[l->cursor++];
}

static char yarn__lexpeek(yarn__lexer *l, int shift) {
    if ((l->cursor + shift) >= l->source.length) return 0;
    if ((l->cursor + shift) < 0)                 return 0;
    return l->source.memory[l->cursor + shift];
}

typedef struct {
    int type;
    int line;
    int grapheme_column;
} yarn__ast;

typedef struct {
    yarn_allocator allocator;
    yarn__ast      *nodes;
} yarn__compiler;

static int yarn__parse_ident(yarn__lexer *l) {
    l->cursor -= 1;
    return 0;
}

static int yarn__parse_digit(yarn__lexer *l) { /* Sets number member inside lexer. */
    /* Begins after consumed one character, so I have to rollback once */
    l->cursor -= 1;

    char c = yarn__lexadvance(l);
    int before_fraction = 0;
    while(c) {
        if(yarn__isdigit(c)) {
            int num = (c - '0');
            if (before_fraction > (INT_MAX - num) / 10) {
                /* Overflow */
                return 0;
            }

            before_fraction = before_fraction * 10 + (c - '0');
        } else { /* Could be a fraction?? */
            break;
        }

        c = yarn__lexadvance(l);
    }

    int after_fraction  = 0;
    int power = 1;
    if (c == '.') { /* It IS a fraction! */
        c = yarn__lexadvance(l);

        while(c) {
            if(yarn__isdigit(c)) {
                int num = (c - '0');
                if (after_fraction > (INT_MAX - num) / 10) {
                    /* Overflow */
                    return 0;
                }

                after_fraction = after_fraction * 10 + (c - '0');
                power *= 10;

                c = yarn__lexadvance(l);
            } else { /* fraction handle is done. */
                l->cursor -= 1; /* Step back once so I don't accidentally eat the breaking character. */
                break;
            }
        }

        l->number = before_fraction + ((float)after_fraction / (float)power);
    } else { /* isn't a fraction. just store the number */
        l->number = before_fraction;
    }

    return 1;
}

enum {
    YARN_TOK_INVALID = 0,

    YARN_TOK_ESCAPE,  /*  \ */
    YARN_TOK_COMMENT, /* // */
    YARN_TOK_NEWLINE,
    YARN_TOK_WHITESPACE,

    /* Basic data literals. */
    /* Identifier detection works on parser level, not on tokenizer */
    YARN_TOK_TEXT,
    YARN_TOK_NUMBER,
    YARN_TOK_STRING,

    YARN_TOK_NODE_BEGIN, /* --- */
    YARN_TOK_NODE_END,   /* === */

    YARN_TOK_COMMAND_BEGIN,  /* << */
    YARN_TOK_COMMAND_END,    /* >> */

    YARN_TOK_OPTION_ARROW,   /* -> */

    YARN_TOK_HASHTAG,        /* # */

    /* Predefined types. */
    YARN_TOK_TYPE_BOOL,
    YARN_TOK_TYPE_NUMBER,
    YARN_TOK_TYPE_STRING,

    /* Reserved command keywords */
    YARN_TOK_SET,
    YARN_TOK_JUMP,
    YARN_TOK_TO,
    YARN_TOK_DECLARE,

    /* control flow keywords */
    YARN_TOK_IF,
    YARN_TOK_ELSEIF,
    YARN_TOK_ELSE,
    YARN_TOK_ENDIF,

    /* misc keywords */
    YARN_TOK_AS,

    /* Control flow stuff */
    YARN_TOK_INDENT,
    YARN_TOK_DEDENT,

    /* Expressions */
    YARN_TOK_COLON,
    YARN_TOK_SEMICOLON,

    YARN_TOK_COMMA,
    YARN_TOK_CARET,
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


static int yarn__lex(yarn__compiler *compiler, yarn__lexer *lexer) {
    /* TODO: @Robustness
     * this assumes that char is unsigned. */
    yarn__lexprep(lexer);
    char c = yarn__lexadvance(lexer);

    switch(c) {
        case '\r':
        {
            if (yarn__lexpeek(lexer, 0) != '\n') { /* If not \r\n, break. otherwise fallthrough */
                return YARN_TOK_INVALID;
            }
        } /* fallthrough */;

        case '\n':
        {
            lexer->line++;
            lexer->grapheme_column = 0;
            return YARN_TOK_NEWLINE;
        } break;

        case ' ':  return YARN_TOK_WHITESPACE; break; /* TODO: indentation */
        case ':': return YARN_TOK_COLON;       break;
        case '#': return YARN_TOK_HASHTAG;     break;

        case '(': return YARN_TOK_OPENPAREN;   break;
        case ')': return YARN_TOK_CLOSEPAREN;  break;
        case '*': return YARN_TOK_ASTERISK;    break;
        case '%': return YARN_TOK_PERCENT;     break;
        case '+': return YARN_TOK_PLUS;        break;
        case '^': return YARN_TOK_CARET;       break;
        case ',': return YARN_TOK_COMMA;       break;
        case '$': return YARN_TOK_DOLLAR;      break;
        case '{': return YARN_TOK_OPENBRACK;   break;
        case '}': return YARN_TOK_CLOSEBRACK;  break;

        case '<':
        {
            int peeked = yarn__lexpeek(lexer, 0);
            if (peeked == '<') {
                lexer->cursor++;
                return YARN_TOK_COMMAND_BEGIN;
            } else if (peeked == '=') {
                lexer->cursor++;
                return YARN_TOK_LESSEQ;
            } else {
                return YARN_TOK_LESS;
            }
        } break;

        case '>':
        {
            int peeked = yarn__lexpeek(lexer, 0);
            if (peeked == '>') {
                lexer->cursor++;
                return YARN_TOK_COMMAND_END;
            } else if (peeked == '=') {
                lexer->cursor++;
                return YARN_TOK_MOREEQ;
            } else {
                return YARN_TOK_MORE;
            }
        } break;

        case '-':
        {
            if (yarn__lexpeek(lexer, 0) == '-' && yarn__lexpeek(lexer, 1) == '-') {
                lexer->cursor += 2;
                return YARN_TOK_NODE_BEGIN;
            } else if (yarn__lexpeek(lexer, 0) == '>') {
                yarn__lexadvance(lexer);
                return YARN_TOK_OPTION_ARROW;
            } else {
                return YARN_TOK_HYPHEN;
            }
        } break;

        case '=':
        {
            if (yarn__lexpeek(lexer, 0) == '=') {
                if(yarn__lexpeek(lexer, 1) == '=') {
                    lexer->cursor += 2;
                    return YARN_TOK_NODE_END;
                } else {
                    lexer->cursor += 1;
                    return YARN_TOK_EQEQ;
                }
            } else {
                return YARN_TOK_EQUAL;
            }
        } break;


        default:
        {
            if (yarn__isdigit(c)) {
                if (yarn__parse_digit(lexer)) {
                    return YARN_TOK_NUMBER;
                }
            } else {
                return yarn__parse_ident(lexer);
            }
        } break;
    }

    return 1;
}

static int yarn__parse(yarn__compiler *compiler, yarn_source source) {
    yarn__lexer lexer = {0};
    lexer.source = source;

    yarn__lexclear(&lexer);

    printf("Lexing %s\n", lexer.source.filename);
    while(1) {
        int token = yarn__lex(compiler, &lexer);
        printf("Token: %d\n", token);

        if (token == YARN_TOK_TEXT && lexer.str_length > 0) {
            printf("Ident: %.*s\n", lexer.str_length, lexer.string);
        }
        if (token == YARN_TOK_NUMBER) {
            printf("Number: %f\n", lexer.number);
        }

        if (token == YARN_TOK_INVALID) {
            break;
        }
    }
}

yarn_program yarn_compile(yarn_compilation_job *job) {
    yarn__compiler compiler = {0};
    yarn_program program = {0};
    compiler.allocator = yarn_create_allocator(8 * 1024);

    for (int i = 0; i < job->sources.used; ++i) {
        yarn__parse(&compiler, job->sources.entries[i]);
    }

    yarn_destroy_allocator(compiler.allocator);
    return program;
}

#endif
