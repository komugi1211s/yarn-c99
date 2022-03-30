

#include "protobuf-c.c"
#include "yarn_spinner.pb-c.h"
#include "yarn_spinner.pb-c.c"

#define YARN_C99_IMPLEMENTATION
#define YARN_C99_STUB_TO_NOOP
#include "yarn_c99.h"

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

void yarn_handle_line(yarn_dialogue *dialogue, yarn_line *line) {
    size_t id_size = strlen(line->id);
    /* char *message = yarn_get_localized_line(locale, line); */
    char *message = yarn_load_line(dialogue, line);
    if (message) {
        printf("%s", message);
    }
    char a = 0;
    while ((a = getchar()) && a != '\n') {}
    yarn_continue(dialogue);
}

void yarn_handle_option(yarn_dialogue *dialogue, yarn_option *options, int option_count) {
    int max_option_id = -1;
    for (int o = 0; o < option_count; ++o) {
        yarn_option *opt = &options[o];

        size_t id_size = strlen(opt->line.id);
        char *message_for_id = 0;

        for (int i = 0; i < dialogue->strings.used; ++i) {
            if (strncmp(dialogue->strings.entries[i].id, opt->line.id, id_size) == 0) {
                message_for_id = dialogue->strings.entries[i].text;
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
            yarn_select_option(dialogue, a);
            break;
        }
    }
}

void yarn_handle_command(yarn_dialogue *dialogue, char *cmd) {
    printf("Command fired: %s\n", cmd);
    yarn_continue(dialogue);
}

void load_program(yarn_dialogue *dialogue, char *yarnc_name) {
    size_t yarn_c_size = 0;
    char *yarn_c = read_entire_file(yarnc_name, &yarn_c_size);

    yarn_load_program(dialogue, yarn_c, yarn_c_size);
}

void load_string_table(yarn_string_table *strtable, char *csv_name) {
    size_t csv_file_size = 0;
    char *csv_data = read_entire_file(csv_name, &csv_file_size);

    yarn_load_csv(strtable, csv_data, csv_file_size);

    free(csv_data);
}

int main(int argc, char **argv) {
    /* creates locale with 32 kb buffer. */
    yarn_variable_storage storage;
    /* yarn_string_table *string_table; */
    yarn_dialogue     *dialogue;

    storage      = yarn_create_default_storage(NULL);
    /* string_table = yarn_create_string_table(NULL); */
    dialogue     = yarn_create_dialogue(NULL, storage);

    char *program_name = (argc > 1) ? argv[1] : "Output.yarnc";
    char *csv_name     = (argc > 2) ? argv[2] : "Output.csv";
    load_program(dialogue, program_name);
    load_string_table(0, csv_name);

    /* dialogue->strings        = *string_table; */
    dialogue->line_handler   = yarn_handle_line;
    dialogue->option_handler = yarn_handle_option;
    dialogue->command_handler = yarn_handle_command;

    char *first_node_name = (argc > 3) ? argv[3] : "Start";
    yarn_set_node(dialogue, first_node_name);

    do {
        yarn_continue(dialogue);
    } while (yarn_is_active(dialogue));

    yarn_destroy_dialogue(dialogue);
    /* yarn_destroy_string_table(string_table); */
    yarn_destroy_default_storage(storage);
    return 0;
}
