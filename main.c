/* nuklear - 1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

struct nk_colorf palette[8 * 16];
int use_palette; // TODO
nk_bool show_layer_1;
nk_bool show_layer_2;

int main(int argc, char *argv[])
{
    /* Platform */
    SDL_Window *win;
    SDL_GLContext glContext;
    int win_width, win_height;
    int running = 1;

    /* GUI */
    struct nk_context *ctx;
    struct nk_colorf bg;

    NK_UNUSED(argc);
    NK_UNUSED(argv);

    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Crane",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &win_width, &win_height);

    /* OpenGL setup */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to setup GLEW\n");
        exit(1);
    }

    ctx = nk_sdl_init(win);
    {
      struct nk_font_atlas *atlas;
      nk_sdl_font_stash_begin(&atlas);
      nk_sdl_font_stash_end();
    }

    ctx->style.combo.content_padding = nk_vec2(1, 1);
    ctx->style.combo.sym_normal = NK_SYMBOL_NONE;
    ctx->style.combo.sym_hover = NK_SYMBOL_NONE;
    ctx->style.combo.sym_active = NK_SYMBOL_NONE;

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    while (running)
    {
        /* Input */
        SDL_Event evt;
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) goto cleanup;
            nk_sdl_handle_event(&evt);
        }
        nk_sdl_handle_grab(); /* optional grabbing behavior */
        nk_input_end(ctx);

        /* GUI */
        if (nk_begin(ctx, "Palette", nk_rect(16, 16, 368, 224),
              NK_WINDOW_BORDER
            | NK_WINDOW_MOVABLE
            | NK_WINDOW_MINIMIZABLE
            | NK_WINDOW_TITLE))
        {
          int row, col;
          for (row = 0; row < 8; row++) {
            char label[2] = { 0 };
            nk_layout_row_static(ctx, 16, 16, 17);
            label[0] = row + '0';
            nk_label(ctx, label, NK_TEXT_CENTERED);
            for (col = 0; col < 16; col++) {
              struct nk_colorf nkc = palette[row * 16 + col];

              if (nk_combo_begin_color(ctx, nk_rgb_cf(nkc), nk_vec2(200, 400))) {
                nk_layout_row_dynamic(ctx, 120, 1);
                nkc = nk_color_picker(ctx, nkc, NK_RGB);
                nk_layout_row_dynamic(ctx, 24, 1);
                nkc.r = nk_propertyf(ctx, "#R:", 0, nkc.r, 1.0f, 0.01f, 0.005f);
                nkc.g = nk_propertyf(ctx, "#G:", 0, nkc.g, 1.0f, 0.01f, 0.005f);
                nkc.b = nk_propertyf(ctx, "#B:", 0, nkc.b, 1.0f, 0.01f, 0.005f);
                palette[row * 16 + col] = nkc;
                nk_combo_end(ctx);
              }
            }
          }

        }
        nk_end(ctx);

        if (nk_begin(ctx, "Background", nk_rect(544, 16, 512, 512),
              NK_WINDOW_BORDER
            | NK_WINDOW_MOVABLE
            | NK_WINDOW_SCALABLE
            | NK_WINDOW_MINIMIZABLE
            | NK_WINDOW_TITLE))
        {
          nk_layout_row_static(ctx, 16, 80, 3);
          nk_label(ctx, "Show layer", NK_TEXT_LEFT);
          nk_checkbox_label(ctx, "1", &show_layer_1);
          nk_checkbox_label(ctx, "2", &show_layer_2);

        }
        nk_end(ctx);

        if (nk_begin(ctx, "Tile", nk_rect(16, 368, 256, 256),
              NK_WINDOW_BORDER
            | NK_WINDOW_MOVABLE
            | NK_WINDOW_SCALABLE
            | NK_WINDOW_MINIMIZABLE
            | NK_WINDOW_TITLE))
        {
          nk_layout_row_dynamic(ctx, 16, 2);
          nk_label(ctx, "Use palette", NK_TEXT_LEFT);
          nk_property_int(ctx, "##use_palette", 0, &use_palette, 7, 1, 1);
        }
        nk_end(ctx);

        /* Draw */
        SDL_GetWindowSize(win, &win_width, &win_height);
        glViewport(0, 0, win_width, win_height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(bg.r, bg.g, bg.b, bg.a);
        /* IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
         * with blending, scissor, face culling, depth test and viewport and
         * defaults everything back into a default state.
         * Make sure to either a.) save and restore or b.) reset your own state after
         * rendering the UI. */
        nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
        SDL_GL_SwapWindow(win);
    }

cleanup:
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

