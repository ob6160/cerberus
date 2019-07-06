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
#include "log.h"

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
	struct wl_output *output;
  struct wlr_output *data;
  uint32_t x, y;
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
	if (flags & WL_OUTPUT_MODE_CURRENT) {
		struct cerberus_output *output = data;
		output->data->width = width;
	  output->data->height = height;
    output->data->refresh = refresh;
	  swaybg_log(LOG_DEBUG, "Output Width: %d", output->data->width);
    swaybg_log(LOG_DEBUG, "Output Height: %d", output->data->height);
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
    struct cerberus_output *output = calloc(1, sizeof(struct cerberus_output));
    output->data = calloc(1, sizeof(struct wlr_output));
    output->id = id;
    output->output = wl_registry_bind(reg, id, &wl_output_interface, 1);
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

  wl_list_init(&server.outputs);

  wl_registry_add_listener(server.wl_registry, &registry_listener, &server);
  wl_display_roundtrip(server.wl_display);
  wl_display_dispatch(server.wl_display);

//  wl_display_disconnect(server.wl_display);
  return 0;
}
