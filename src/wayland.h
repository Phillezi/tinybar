#pragma once

#ifndef TINY__WAYLAND_H
#define TINY__WAYLAND_H

#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "wlr-layer-shell-unstable-v1.h"

typedef struct {
  uint_fast8_t dirty;
  uint_fast8_t configured;

  uint32_t width;
  uint32_t height;

  uint32_t bar_height;

  size_t shm_size;
  int shm_fd;
  void *shm_data;

  struct wl_display *display;
  struct wl_compositor *compositor;
  struct wl_shm *shm;
  struct zwlr_layer_shell_v1 *layer_shell;

  struct wl_callback *frame_cb;
  struct wl_buffer *buffer;
  struct wl_surface *surface;

  struct zwlr_layer_surface_v1 *layer_surface;

} TINY__Application;

// LISTENERS

static void tiny__reg_add(void *data, struct wl_registry *r, uint32_t id,
                          const char *iface, uint32_t ver) {
  TINY__Application *app = (TINY__Application *)data;
  (void)ver;

  if (strcmp(iface, "wl_compositor") == 0)
    app->compositor = wl_registry_bind(r, id, &wl_compositor_interface, 4);
  else if (strcmp(iface, "wl_shm") == 0)
    app->shm = wl_registry_bind(r, id, &wl_shm_interface, 1);
  else if (strcmp(iface, "zwlr_layer_shell_v1") == 0)
    app->layer_shell =
        wl_registry_bind(r, id, &zwlr_layer_shell_v1_interface, 1);
}

static void tiny__reg_remove(void *data, struct wl_registry *r, uint32_t id) {
  (void)data;
  (void)r;
  (void)id;
}

static void tiny__frame_done(void *d, struct wl_callback *cb, uint32_t t) {
  TINY__Application *app = (TINY__Application *)d;
  (void)t;

  wl_callback_destroy(cb);
  app->frame_cb = NULL;
}

static void tiny__configure(void *data, struct zwlr_layer_surface_v1 *l,
                            uint32_t serial, uint32_t w, uint32_t h) {
  TINY__Application *app = (TINY__Application *)data;

  if (w)
    app->width = w;
  if (h)
    app->height = h;

  zwlr_layer_surface_v1_ack_configure(l, serial);

  if (!app->configured) {
    app->configured = 1;
    // create_buffer();
    // redraw();
  }
}

static const struct wl_registry_listener tiny__reg_listener = {
    .global = tiny__reg_add, .global_remove = tiny__reg_remove};

static const struct wl_callback_listener tiny__frame_listener = {
    .done = tiny__frame_done};

static const struct zwlr_layer_surface_v1_listener tiny__layer_listener = {
    .configure = tiny__configure, .closed = NULL};

static int tiny__create_shm_fd() {
  return syscall(SYS_memfd_create, "tinybar", 0);
}

static int tiny__create_buffer(TINY__Application *app) {
  app->shm_size = (size_t)app->width * (size_t)app->height *
                  4; // STRIDE is hardcoded to 4 here?

  app->shm_fd = tiny__create_shm_fd();
  if (app->shm_fd < 0)
    return 1;

  ftruncate(app->shm_fd, app->shm_size);

  app->shm_data = mmap(NULL, app->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       app->shm_fd, 0);

  if (app->shm_data == MAP_FAILED) {
    app->shm_data = NULL;
    return 1;
  }

  struct wl_shm_pool *pool =
      wl_shm_create_pool(app->shm, app->shm_fd, app->shm_size);

  app->buffer = wl_shm_pool_create_buffer(
      pool, 0, app->width, app->height, app->width * 4,
      WL_SHM_FORMAT_ARGB8888); // stride hardcoded to 4

  wl_shm_pool_destroy(pool);

  return 0;
}

static inline void tiny__cleanup(TINY__Application *app) {
  if (app->frame_cb) {
    wl_callback_destroy(app->frame_cb);
    app->frame_cb = NULL;
  }

  if (app->buffer) {
    wl_buffer_destroy(app->buffer);
    app->buffer = NULL;
  }

  if (app->layer_surface) {
    zwlr_layer_surface_v1_destroy(app->layer_surface);
    app->layer_surface = NULL;
  }

  if (app->surface) {
    wl_surface_destroy(app->surface);
    app->surface = NULL;
  }

  if (app->shm_data && app->shm_size) {
    munmap(app->shm_data, app->shm_size);
    app->shm_data = NULL;
  }

  if (app->shm_fd >= 0) {
    close(app->shm_fd);
    app->shm_fd = -1;
  }

  if (app->shm) {
    wl_shm_destroy(app->shm);
    app->shm = NULL;
  }

  if (app->compositor) {
    wl_compositor_destroy(app->compositor);
    app->compositor = NULL;
  }

  if (app->layer_shell) {
    zwlr_layer_shell_v1_destroy(app->layer_shell);
    app->layer_shell = NULL;
  }

  if (app->display) {
    wl_display_disconnect(app->display);
    app->display = NULL;
  }
}

static void tiny__schedule_frame(TINY__Application *app) {
  if (app->frame_cb)
    return;
  app->frame_cb = wl_surface_frame(app->surface);
  wl_callback_add_listener(app->frame_cb, &tiny__frame_listener, NULL);
}

// Initialize
static inline int tiny__init(TINY__Application *app) {
  app->display = wl_display_connect(NULL);
  if (!app->display)
    return 1;

  struct wl_registry *r = wl_display_get_registry(app->display);
  wl_registry_add_listener(r, &tiny__reg_listener, (void *)app);
  wl_display_roundtrip(app->display);

  if (!app->compositor || !app->shm || !app->layer_shell) {
    tiny__cleanup(app);
    return 1;
  }

  app->surface = wl_compositor_create_surface(app->compositor);

  app->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      app->layer_shell, app->surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_TOP,
      "tinybar");

  zwlr_layer_surface_v1_add_listener(app->layer_surface, &tiny__layer_listener,
                                     (void *)app);

  zwlr_layer_surface_v1_set_anchor(app->layer_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

  zwlr_layer_surface_v1_set_size(app->layer_surface, 0, app->bar_height);
  zwlr_layer_surface_v1_set_exclusive_zone(app->layer_surface, app->bar_height);

  wl_surface_commit(app->surface);

  /* ensure configure is processed */
  wl_display_roundtrip(app->display);

  return 0;
}

#endif // !TINY__WAYLAND_H
