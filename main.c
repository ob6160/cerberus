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

struct cerberus_server {
  struct wl_display *wl_display;
  struct wl_registry *wl_registry;
};

void global_add(void *our_data,
        struct wl_registry *registry,
        uint32_t name,
        const char *interface,
        uint32_t version) {
    // TODO
}

void global_remove(void *our_data,
        struct wl_registry *registry,
        uint32_t name) {
    // TODO
}

struct wl_registry_listener registry_listener = {
    .global = global_add,
    .global_remove = global_remove
};


int main(int argc, char **argv) {
  swaybg_log_init(LOG_DEBUG);
  swaybg_log(LOG_DEBUG, "c %d, v %s", argc, argv[1]);

  // Setup Cerberus
  struct cerberus_server server;

  // Connect to wayland display server
  server.wl_display = wl_display_connect(NULL);
  assert(server.wl_display);

  // Setup registry
  server.wl_registry = wl_display_get_registry(server.wl_display);
  assert(server.wl_registry);

  wl_display_disconnect(server.wl_display);
  return 0;
}
