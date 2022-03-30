#include <raylib.h>

#include "protobuf-c.c"
#include "yarn_spinner.pb-c.h"
#include "yarn_spinner.pb-c.c"
#define YARN_C99_IMPLEMENTATION
#include "yarn_c99.h"

enum {
    MD_NONE,
    MD_LINE,
    MD_OPTION,
};

static int mode = MD_NONE;

static char  *sentence        = 0;
static size_t sentence_count  = 0;
static size_t sentence_length = 0;

static yarn_option *options   = 0;
static int          n_options = 0;

static float accumulator = 0;
static Font font;
#define TEXT_SIZE 20

void handle_line(yarn_dialogue *dialog, yarn_line *line) {
    mode = MD_LINE;
    accumulator = 0;
 
    char *text = yarn_load_line(dialog, line);
    sentence = text;
    sentence_count = 0;
    sentence_length = strlen(text)+1;
}

void handle_options(yarn_dialogue *dialogue, yarn_option *opt, int n_opt) {
    mode = MD_OPTION;
    accumulator = 0;

    options   = opt;
    n_options = n_opt;
}

void render_initial_prompt(yarn_dialogue *d) {
    Vector2 pos = {
        10, 10
    };

    DrawTextEx(font, "Press X to start `Sally` dialog.\n", pos, TEXT_SIZE, 0, BLACK);

    pos.y += TEXT_SIZE + 10;
    DrawTextEx(font, "Press Y to start `Ship`  dialog.\n", pos, TEXT_SIZE, 0, BLACK);

    if (IsKeyPressed('X')) {
        yarn_set_node(d, "Sally");
        yarn_continue(d);
    } else if (IsKeyPressed('Y')) {
        yarn_set_node(d, "Ship");
        yarn_continue(d);
    }
}

void render_sentence(yarn_dialogue *d, int x, int y) {
    int reached_to_an_end = (sentence_count == sentence_length);

    /* ===============
     * Update.
     * =============== */
    if (IsKeyPressed(KEY_ENTER)) {
        if (sentence_count == sentence_length) {
            mode = MD_NONE;
            yarn_continue(d);
            return;
        } else {
            sentence_count = sentence_length - 2;
        }
    }


    /* ===============
     * Render.
     * =============== */
    if (reached_to_an_end) {
        char prompt[] = "next (ENTER)";
        int prompt_width = MeasureTextEx(font, prompt, TEXT_SIZE, 0).x;

        int prompt_x = (GetScreenWidth() - prompt_width) / 2;
        int prompt_y = GetScreenHeight() - TEXT_SIZE - 10; /* a bit of margin here. */

        Vector2 position = {
            prompt_x,
            prompt_y,
        };

        DrawTextEx(font, prompt, position, TEXT_SIZE, 0, BLACK);
    }

    char *f = sentence;
    if (!reached_to_an_end) f = strndup(sentence, sentence_count);
    Vector2 position = {
        x,
        y,
    };
    DrawTextEx(font, f, position, TEXT_SIZE, 0, BLUE);
    if (!reached_to_an_end) free(f);
}

void render_options(yarn_dialogue *d, int x, int y) {
    /* ===============
     * Update.
     * =============== */
    for (int i = 0; i < n_options; ++i) {
        yarn_option *o = &options[i];
        if (o->is_available) {
            assert(n_options <= 9);
            int key = o->id + '0';

            if (IsKeyPressed(key)) {
                mode = MD_NONE;
                yarn_select_option(d, o->id);
                yarn_continue(d);
                return;
            }
        }
    }

    Vector2 pos = { x, y };

    /* ===============
     * Render.
     * =============== */
    for (int i = 0; i < n_options; ++i) {
        yarn_option *o = &options[i];
        char *line = yarn_load_line(d, &o->line);
        const char *prompt;
        Color color;

        if (!o->is_available) {
            color = Fade(BLACK, 0.5);
            prompt = TextFormat("%d: %s (unavailable)", o->id, line);
        } else {
            color = BLACK;
            prompt = TextFormat("%d: %s", o->id, line);
        }

        DrawTextEx(font, prompt, pos, TEXT_SIZE, 0, color);
        pos.y += TEXT_SIZE + 5;
    }
}

void render_debug_info(yarn_dialogue *dialogue) {
    Color   color = Fade(BLACK, 0.5);
    Vector2 dialogue_pos = { GetScreenWidth() - MeasureTextEx(font, "variable:", TEXT_SIZE, 0).x - 10, 0 };

    DrawTextEx(font, "variable:", dialogue_pos, TEXT_SIZE, 0, color);
    dialogue_pos.y += TEXT_SIZE;

    for (int i = 0; i < dialogue->variables.used; ++i) {
        yarn_variable_entry *var = &dialogue->variables.entries[i];
        const char *text;

        switch(var->value.type) {
            case YARN_VALUE_BOOL:   text = TextFormat("%u: %s", var->hash, (var->value.values.v_bool ? "true": "false")); break;
            case YARN_VALUE_STRING: text = TextFormat("%u: %s", var->hash,    var->value.values.v_string); break;
            case YARN_VALUE_FLOAT:  text = TextFormat("%u: %2.0f", var->hash, var->value.values.v_float); break;
            default:                text = TextFormat("%u: None", var->hash); break;
        }

        dialogue_pos.x = GetScreenWidth() - MeasureTextEx(font, text, TEXT_SIZE, 0).x - 10;
        DrawTextEx(font, text, dialogue_pos, TEXT_SIZE, 0, color);
        dialogue_pos.y += TEXT_SIZE;
    }
}

int main(int argc, char **argv) {
    InitWindow(800, 600, "Yarn Example");
    yarn_dialogue *d = yarn_create_dialogue_heap(0);

    /* fonts */
    font = LoadFontEx("resources/Inconsolata.ttf", TEXT_SIZE, 0, 0);

    /* Read files */
    unsigned int yarnc_read = 0;
    unsigned int csv_read = 0;
    char *yarnc_file = (char*)LoadFileData("resources/Output.yarnc", &yarnc_read);
    char *csv_file   = (char*)LoadFileData("resources/Output.csv",   &csv_read);

    assert(yarnc_file && csv_file);
    yarn_load_program(d, yarnc_file, (size_t)yarnc_read, csv_file, (size_t)csv_read);

    UnloadFileData((unsigned char*)yarnc_file);
    UnloadFileData((unsigned char*)csv_file);

    d->delegates.line_handler   = handle_line;
    d->delegates.option_handler = handle_options;

    float char_per_second = 24;
    float char_interval_in_ms = 1000 / char_per_second;

    while (!WindowShouldClose()) {
        float dialog_width  = GetScreenWidth()  * 0.75;
        float dialog_height = GetScreenHeight() * 0.15;

        int x = (GetScreenWidth() - dialog_width) * 0.5;
        int y = GetScreenHeight() * 0.75;

        BeginDrawing();
        ClearBackground(WHITE);

        render_debug_info(d);

        if (!yarn_is_active(d)) {
            render_initial_prompt(d);
        } else {
            accumulator += 1000 * GetFrameTime();

            if (accumulator > char_interval_in_ms) {
                accumulator -= char_interval_in_ms;
                if (accumulator < 0) accumulator = 0;

                sentence_count += 1;
            }

            if (sentence_length != 0 && (sentence_count > sentence_length)) {
                sentence_count = sentence_length;
            }

            switch(mode) {
                case MD_LINE:   render_sentence(d, x, y); break;
                case MD_OPTION: render_options(d, x, y); break;
                default: break;
            }

            switch(mode) {
                case MD_LINE:
                {
                } break;
            }
        }
        EndDrawing();
    }

    yarn_destroy_dialogue(d);
    UnloadFont(font);
    CloseWindow();
    return 0;
}
