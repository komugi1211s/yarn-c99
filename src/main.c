/*
 ==========================================
  Example code for current yarn spinner.
  jump to "main" function below
 ==========================================
*/

/*
 changing this define to 0 will still compile,
 but will cause a link error.
*/

#define COMPILE_WITH_PROTOBUF 1

#if COMPILE_WITH_PROTOBUF
#include "protobuf-c.c"
#include "yarn_spinner.pb-c.h"
#include "yarn_spinner.pb-c.c"

#define YARN_C99_IMPLEMENTATION
#else
#include <stdlib.h>
#include <stdio.h>
#endif
#include "yarn_c99.h"

/*
 ==========================================
  Forward declaration of functions used in example.
 ==========================================
 */
static void yarn_handle_line(yarn_dialogue *dialogue, yarn_line *line);
static void yarn_handle_option(yarn_dialogue *dialogue, yarn_option *options, int option_count);
static void yarn_handle_command(yarn_dialogue *dialogue, char *cmd);

static char *read_entire_file(char *file_name, size_t *bytes_read);
static void load_program(yarn_dialogue *dialogue, char *yarnc_name);
static void load_string_table(yarn_string_table *strtable, char *csv_name);

/*
 ==========================================
  Main function
 ==========================================
 */
int main(int argc, char **argv) {
    yarn_variable_storage storage;
    yarn_string_table *string_table;
    yarn_dialogue     *dialogue;

    /* setup your project. */
    storage      = yarn_create_default_storage();
    string_table = yarn_create_string_table();
    dialogue     = yarn_create_dialogue(storage);

    /* load yarnc/csv to your dialogue. */
    char *program_name = (argc > 1) ? argv[1] : "Output.yarnc";
    char *csv_name     = (argc > 2) ? argv[2] : "Output.csv";
    load_program(dialogue, program_name);
    load_string_table(string_table, csv_name);

    /* assign ops respectively. */
    dialogue->strings        = string_table;

    /* dialogue->log_debug = imaginary_debug_function; -- Optional */
    /* dialogue->log_error = imaginary_error_function; -- Optional */
    dialogue->line_handler   = yarn_handle_line;
    dialogue->option_handler = yarn_handle_option;
    dialogue->command_handler = yarn_handle_command;

    /* set up your first node, then... */
    char *first_node_name = (argc > 3) ? argv[3] : "Start";
    yarn_set_node(dialogue, first_node_name);

    /* start. */
    do {
        yarn_continue(dialogue);
    } while (yarn_is_active(dialogue));

    /* when done, cleanup. */
    yarn_destroy_dialogue(dialogue);
    yarn_destroy_string_table(string_table);
    yarn_destroy_default_storage(storage);
    return 0;
}
/*
 ==========================================
  Implementation of declared functions.
 ==========================================
 */

void yarn_handle_line(yarn_dialogue *dialogue, yarn_line *line) {
    /* create displayable line (performs substitution of {0}, {1}...) */
    /* note: line must be freed on your own */

    char *message  = yarn_convert_to_displayable_line(dialogue->strings, line);

    if (message) {
        printf("%s", message);
    }
    char a = 0;
    while ((a = getchar()) && a != '\n') {}

    /* when done, clear message and continue. */
    yarn_destroy_displayable_line(message);

    /* NOTE: data inside line structure is alive until the current dialogue is done
     * or node jump happens (or until any "dialogue-complete" command gets called) */
    yarn_continue(dialogue);
}

void yarn_handle_option(yarn_dialogue *dialogue, yarn_option *options, int option_count) {
    for (int o = 0; o < option_count; ++o) {
        yarn_option *opt = &options[o];
        /* create displayable line (performs substitution of {0}, {1}...) */
        char *message_for_id = yarn_convert_to_displayable_line(dialogue->strings, &opt->line);

        printf("%d: %s\n", options[o].id, message_for_id);

        /* when done, clear message and continue. */
        yarn_destroy_displayable_line(message_for_id);
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
            yarn_select_option(dialogue, a);
            break;
        }
    }
}

void yarn_handle_command(yarn_dialogue *dialogue, char *cmd) {
    printf("Command fired: %s\n", cmd);
    yarn_continue(dialogue);
}

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

void load_program(yarn_dialogue *dialogue, char *yarnc_name) {
    size_t yarn_c_size = 0;
    char *yarn_c = read_entire_file(yarnc_name, &yarn_c_size);

    yarn_load_program(dialogue, yarn_c, yarn_c_size);
    free(yarn_c);
}

void load_string_table(yarn_string_table *strtable, char *csv_name) {
    size_t csv_file_size = 0;
    char *csv_data = read_entire_file(csv_name, &csv_file_size);

    yarn_load_string_table(strtable, csv_data, csv_file_size + 1);
    free(csv_data);
}
