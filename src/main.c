#define _GNU_SOURCE

#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>

#include <sys/timerfd.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "blocks.h"
#include "clock.h"
#include "hyprland.h"
#include "wayland.h"
#include "workspaces.h"

#include "draw.h"
#include "module.h"

static volatile int running = 1;
static void handle_sigint(int sig) {
  (void)sig;
  running = 0;
}

int main() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigint;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  TINY__Application app = {1,    0,    0,    0,    15,   0,    -1,   NULL,
                           NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

  if (tiny__init(&app)) {
    tiny__cleanup(&app);
    return 1;
  }

  tiny__build_scaled_font();

  while (!app.configured && running) {
    if (!running) {
      break;
    }
    usleep(500000);
  }

  if (app.configured) {
    if (tiny__create_buffer(&app)) {
      tiny__cleanup(&app);
      return 1;
    }
  }

  int clock_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (clock_fd < 0) {
    tiny__cleanup(&app);
    return 1;
  }
  int hypr_fd = tiny__init_hypr_socket(1);
  int wlfd = wl_display_get_fd(app.display);

  struct itimerspec ts;
  ts.it_interval.tv_sec = 1; // repeat interval
  ts.it_interval.tv_nsec = 0;
  ts.it_value.tv_sec = 1; // initial expiration
  ts.it_value.tv_nsec = 0;

  if (timerfd_settime(clock_fd, 0, &ts, NULL) < 0) {
    tiny__cleanup(&app);
    return 1;
  }

  struct pollfd fds[3];

  fds[0].fd = wlfd;
  fds[0].events = POLLIN;

  fds[1].fd = clock_fd;
  fds[1].events = POLLIN;

  fds[2].fd = hypr_fd;
  fds[2].events = (hypr_fd >= 0) ? POLLIN : 0;

  static clock_state_t clock_state;
  static ws_state_t ws_state;
  static blocks_state_t blocks_state = {"[][][][0]"};

  static bar_module_t modules[] = {
      {tiny__ws_update, tiny__ws_draw, 0, &ws_state},
      {tiny__clock_update, tiny__clock_draw, 0, &clock_state},
      {tiny__blocks_update, tiny__blocks_draw, 0, &blocks_state}};

  static const int module_count = sizeof(modules) / sizeof(modules[0]);

  static char workspaces[128] = "WS ?";
  static size_t workspaces_size = sizeof(workspaces);
  static HyprWorkspace hypr_ws[MAX_WORKSPACES];

  // initial
  tiny__clock_update(&modules[1]);
  int count = tiny__query_workspaces(hypr_ws);
  if (count >= 0) {
    tiny__workspaces_to_string(hypr_ws, workspaces, workspaces_size);
    snprintf(ws_state.text, sizeof(ws_state.text), "%s", workspaces);
    app.dirty = 1;
  }
  tiny__blocks_update(&modules[2]);

  tiny__draw_modules(&app, modules, module_count, app.width);

  wl_surface_attach(app.surface, app.buffer, 0, 0);
  wl_surface_damage_buffer(app.surface, 0, 0, app.width,
                           app.height); // TODO: track damage
  wl_surface_commit(app.surface);

  tiny__schedule_frame(&app);

  wl_display_flush(app.display);

  while (running) {
    int ret = poll(fds, (hypr_fd >= 0) ? 3 : 2, 150);
    if (ret < 0) {
      continue;
    }

    if (fds[1].revents & POLLIN) {
      uint64_t expirations;
      read(clock_fd, &expirations, sizeof(expirations)); // clear the event
      tiny__clock_update(&modules[1]);
      app.dirty = 1;
    }

    if (hypr_fd >= 0 && (fds[2].revents & POLLIN)) {
      int count = tiny__query_workspaces(hypr_ws);
      if (count >= 0) {
        if (!tiny__update_workspaces_socket(hypr_fd, hypr_ws)) {
          tiny__workspaces_to_string(hypr_ws, workspaces, workspaces_size);
          snprintf(ws_state.text, sizeof(ws_state.text), "%s", workspaces);
          app.dirty = 1;
        }
      }
    }

    if (fds[0].revents & POLLIN) {
      wl_display_dispatch(app.display);
    }

    if (app.dirty) {
      if (!app.frame_cb) {
        tiny__draw_modules(&app, modules, module_count, app.width);

        wl_surface_attach(app.surface, app.buffer, 0, 0);
        wl_surface_damage_buffer(app.surface, 0, 0, app.width,
                                 app.height); // TODO: track damage
        wl_surface_commit(app.surface);

        tiny__schedule_frame(&app);
        app.dirty = 0;
      } else {
        wl_callback_destroy(app.frame_cb);
        app.frame_cb = NULL;
      }
    }

    wl_display_flush(app.display);
  }

  if (clock_fd >= 0) {
    close(clock_fd);
    clock_fd = -1;
  }
  if (hypr_fd >= 0) {
    close(hypr_fd);
    hypr_fd = -1;
  }

  tiny__cleanup(&app);
}
