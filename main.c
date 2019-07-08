#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wayland-client.h>
#include <wlr/types/wlr_output.h>
#include <stdarg.h>
#include <math.h>


#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#include "log.h"

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 200

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

/**
 * Internal representation of server state
 */
struct cerberus_server {
  struct wl_display *wl_display;
  struct wl_registry *wl_registry; // Global event hndlr, interface data
  struct wl_list outputs; // Handles for each head
};

struct cerberus_output {
	struct wl_list link;
	uint32_t id;
  uint32_t x, y;
  struct wl_output *output;
  struct wlr_output *data;
};

static void output_handle_geometry(void *data, struct wl_output *wl_output,
		int32_t x, int32_t y, int32_t phys_width, int32_t phys_height,
		int32_t subpixel, const char *make, const char *model,
		int32_t transform) {
	
  struct cerberus_output *output = data;
  strncpy(output->data->make, make, sizeof(output->data->make));
	strncpy(output->data->model, model, sizeof(output->data->model));
  
  output->data->phys_width = phys_width;
  output->data->phys_height = phys_height;
  output->data->subpixel = subpixel;
  output->data->transform = transform;

  output->x = x;
  output->y = y;
  
  swaybg_log(LOG_DEBUG, "Output Make: %s", output->data->make);
}

static void output_handle_mode(void *data, struct wl_output *wl_output,
		uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
  
  // Get output context
  struct cerberus_output *output = data;
  // Register and populate output
  struct wlr_output_mode *mode = calloc(1, sizeof(struct wlr_output_mode));
  mode->flags = flags;
  mode->width = width;
  mode->height = height;
  mode->refresh = refresh;
  
  wl_list_insert(&output->data->modes, &mode->link);

	// Get current (enabled) mode.
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    output->data->current_mode = mode;  
  }
}

static void output_handle_done(void* data, struct wl_output *wl_output) {
	/* Nothing to do */
}

static void output_handle_scale(void* data, struct wl_output *wl_output,
		int32_t factor) {
	struct cerberus_output *output = data;
  output->data->scale = factor;
}

/**
 * Output listener
 */
static const struct wl_output_listener output_listener = {
	.geometry = output_handle_geometry,
	.mode = output_handle_mode,
	.done = output_handle_done,
	.scale = output_handle_scale,
};

/**
 * Registry object add event handler.
 */
void global_add(void *data,
        struct wl_registry *reg,
        uint32_t id,
        const char *interface,
        uint32_t version) {
  
	struct cerberus_server *server = data;
  if (!strcmp(interface, wl_output_interface.name)) {
    // Setup output
    struct cerberus_output *output = calloc(1, sizeof(struct cerberus_output));
    output->data = calloc(1, sizeof(struct wlr_output));
    output->id = id;
    output->output = wl_registry_bind(reg, id, &wl_output_interface, 1);

    // Init the mode list.
    wl_list_init(&output->data->modes);


    wl_output_add_listener(output->output, &output_listener, output);
    wl_list_insert(&server->outputs, &output->link);
  }
}

/**
 * Registry object remove event handler.
 */
void global_remove(void *our_data,
        struct wl_registry *registry,
        uint32_t name) {
    // TODO
}

/**
 * We register a registry listener so we know when global
 * events take place. We subscribe on add/remove.
 */
struct wl_registry_listener registry_listener = {
    .global = global_add,
    .global_remove = global_remove
};

static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}

int main(int argc, char **argv) {
  swaybg_log_init(LOG_DEBUG);
  swaybg_log(LOG_DEBUG, "c %d, v %s", argc, argv[1]);

  struct cerberus_server server;

  // Connect to wayland display server
  server.wl_display = wl_display_connect(NULL);
  assert(server.wl_display);

  // Setup registry and listener
  server.wl_registry = wl_display_get_registry(server.wl_display);
  assert(server.wl_registry);

  // Setup linked list of outputs
  wl_list_init(&server.outputs);

  // Setup general i/o registry
  wl_registry_add_listener(server.wl_registry, &registry_listener, &server);

  // Init display
  wl_display_roundtrip(server.wl_display);
  wl_display_dispatch(server.wl_display);

  /* Platform */
  static GLFWwindow *win;
  int width = 0, height = 0;
  struct nk_context *ctx;
  struct nk_colorf bg;
  int window_open = 1;
  /* GLFW */
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
      fprintf(stdout, "[GFLW] failed to init!\n");
      exit(1);
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
  win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Cerberus GUI", NULL, NULL);
  glfwMakeContextCurrent(win);
  glfwGetWindowSize(win, &width, &height);
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

  /* OpenGL */
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glewExperimental = 1;
  if (glewInit() != GLEW_OK) {
      fprintf(stderr, "Failed to setup GLEW\n");
      exit(1);
  }

  ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);

  /* Load Fonts: if none of these are loaded a default font will be used  */
  /* Load Cursor: if you uncomment cursor loading please hide the cursor */
  {struct nk_font_atlas *atlas;
  nk_glfw3_font_stash_begin(&atlas);
  /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
  /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 14, 0);*/
  /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
  /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
  /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
  /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
  nk_glfw3_font_stash_end();}

  char msg[200];
  while (!glfwWindowShouldClose(win)) {
    /* Input */
    glfwPollEvents();
    nk_glfw3_new_frame();
 
    /* GUI */
    if (
      nk_begin(ctx,
      "Cerberus",
      nk_rect(0, 0, width, height),
      NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_TITLE)
    ){
 
      nk_layout_row_begin(ctx, NK_STATIC, 30, 1);
      {
        nk_layout_row_push(ctx, width-30);
        struct cerberus_output *output;
        int i = 0;
        wl_list_for_each(output, &server.outputs, link) {
          sprintf(msg,
            "Monitor %d: %s",
            i,
            output->data->make
          ), nk_label(ctx, msg, NK_TEXT_LEFT);
          sprintf(msg,
            "> Current Mode: %dx%d",
            output->data->current_mode->width,
            output->data->current_mode->height
          ), nk_label(ctx, msg, NK_TEXT_LEFT);
          i++;
        }
        if (nk_button_label(ctx, "button")) {
          fprintf(stdout, "button pressed\n");
        }
      }
      nk_layout_row_end(ctx);
    }
    nk_end(ctx);
    /* Draw */
    glfwGetWindowSize(win, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.25f, 0.5f, 0.5f, 1.0f);
    /* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
    nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
    glfwSwapBuffers(win);
  }
  nk_glfw3_shutdown();
  glfwTerminate();
  wl_display_disconnect(server.wl_display);
  return 0;
}
