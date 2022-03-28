
#define YARN_C99_IMPLEMENTATION
#include "protobuf-c.c"
#include "yarn_spinner.pb-c.h"
#include "yarn_spinner.pb-c.c"
#include "yarn_c99.h"

#include <stdlib.h>
#include <stdio.h>


struct Entity {
    int commands;
};


char *read_entire_file(char *file_name, size_t *bytes_read) {
    assert(file_name && bytes_read);
    FILE *fp = 0;
    fp = fopen(file_name, "rb");
    assert(fp);
    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *result = malloc(filesize + 1);
    assert(result);
    memset(result, 0, filesize+1);

    size_t read = fread(result, 1, filesize, fp);
    assert(read == filesize);
    fclose(fp);

    assert(bytes_read);
    *bytes_read = filesize;
    return result;
}

void yarn_handle_line(yarn_ctx *ctx, yarn_line *line) {
    size_t id_size = strlen(line->id);
    char *message = "unknown";

    for (int i = 0; i < ctx->strings.used; ++i) {
        if (strncmp(ctx->strings.entries[i].id, line->id, id_size) == 0) {
            message = ctx->strings.entries[i].text;
            break;
        }
    }

    printf("%s", message);
    char a = 0;
    while ((a = getchar()) && a != '\n') {}
    yarn_continue(ctx);
}

void yarn_handle_option(yarn_ctx *ctx, yarn_option *options, int option_count) {
    int max_option_id = -1;
    for (int o = 0; o < option_count; ++o) {
        yarn_option *opt = &options[o];

        size_t id_size = strlen(opt->line.id);
        char *message_for_id = 0;

        for (int i = 0; i < ctx->strings.used; ++i) {
            if (strncmp(ctx->strings.entries[i].id, opt->line.id, id_size) == 0) {
                message_for_id = ctx->strings.entries[i].text;
                break;
            }
        }

        printf("%d: %s\n", options[o].id, message_for_id);
    }

    yarn_option *chosen = 0;
    for (;;) {
        int a = getchar() - '0';
        printf("chosen: %d\n", a);

        if (a < 0 || a > 10) {
            printf("That char is unselectable\n");
            continue;
        }

        for (int i = 0; i < option_count; ++i) {
            yarn_option *opt = &options[i];
            if (opt->id == a) {
                chosen = opt;
                break;
            }
        }

        if (chosen) {
            char b = 0;
            while ((b = getchar()) && b != '\n') {}
            yarn_select_option(ctx, a);
            break;
        }
    }
}

void yarn_handle_command(yarn_ctx *ctx, char *cmd) {
    printf("Command fired: %s\n", cmd);
    yarn_continue(ctx);
}

int main(int argc, char **argv) {
    yarn_ctx *ctx = yarn_create_context_heap();

    assert(argc == 3);
    char *yarnc = argv[1];
    char *csv   = argv[2];
    size_t yarnc_size = 0;
    size_t strtable_size = 0;
    char *yarn_c = read_entire_file(yarnc, &yarnc_size);
    char *strtable = read_entire_file(csv, &strtable_size);
    yarn_load_program(ctx, yarn_c, yarnc_size, strtable, strtable_size);

    ctx->delegates.line_handler    = yarn_handle_line;
    ctx->delegates.option_handler  = yarn_handle_option;
    ctx->delegates.command_handler = yarn_handle_command;
    yarn_continue(ctx);

    free(yarn_c);
    free(strtable);
    yarn_destroy_context(ctx);
}
