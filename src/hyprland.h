#pragma once

#ifndef TINY__HYPRLAND_H
#define TINY__HYPRLAND_H

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define MAX_WORKSPACES 16

typedef struct {
  int id;           // workspace ID
  char monitor[32]; // monitor name
  int monitor_id;
  int windows;
  int hasfullscreen;
  unsigned long lastwindow;
  char lastwindowtitle[128];
  int ispersistent;
  char tiledLayout[16];
  int focused; // 1 if this workspace is active
} HyprWorkspace;

static int tiny__parse_workspaces(const char *buf, HyprWorkspace workspaces[],
                                  int max_ws);

static void tiny__workspaces_to_string(HyprWorkspace workspaces[], char *out,
                                       size_t out_size);

// Initialize a Hyprland socket.
// socket2 == 0 => main socket (.socket.sock) for querying
// socket2 != 0 => event socket (.socket2.sock) for reading workspace events
static int tiny__init_hypr_socket(int socket2) {
  const char *sig = getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (!sig)
    return -1;

  const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
  if (!runtime_dir)
    return -1;

  char path[512];
  if (socket2)
    snprintf(path, sizeof(path), "%s/hypr/%s/.socket2.sock", runtime_dir, sig);
  else
    snprintf(path, sizeof(path), "%s/hypr/%s/.socket.sock", runtime_dir, sig);

  int fd;
  if (socket2)
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK,
                0); // non-blocking for event socket
  else
    fd = socket(AF_UNIX, SOCK_STREAM, 0); // blocking for query socket
  if (fd < 0)
    return -1;

  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

  size_t len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

  if (connect(fd, (struct sockaddr *)&addr, len) < 0) {
    if (errno != EINPROGRESS) {
      close(fd);
      return -1;
    }
  }

  return fd;
}

static int tiny__query_workspaces(HyprWorkspace ws_array[MAX_WORKSPACES]) {
  if (!ws_array)
    return -1;

  int fd = tiny__init_hypr_socket(0);
  if (fd < 0) {
    printf("failed to open socket\n");
    return -1;
  }

  const char *cmd = "workspaces";
  write(fd, cmd, strlen(cmd));

  char buf[8192];
  ssize_t n = 0;
  size_t total = 0;

  while ((n = read(fd, buf + total, sizeof(buf) - 1 - total)) > 0)
    total += n;

  if (n < 0)
    perror("read from hypr socket");

  buf[total] = '\0';
  close(fd);

  return tiny__parse_workspaces(buf, ws_array, MAX_WORKSPACES);
}

// Read workspace events from the event socket (.socket2.sock)
// returns 0 if the event was a workspaces event and everything went ok
static int
tiny__update_workspaces_socket(int hypr_fd,
                               HyprWorkspace ws_array[MAX_WORKSPACES]) {
  if (hypr_fd < 0 || !ws_array)
    return 1;

  char buf[1024];
  ssize_t n = read(hypr_fd, buf, sizeof(buf) - 1);
  if (n <= 0)
    return 1;

  buf[n] = '\0';

  int workspace_evt_found = 0;

  char *line = strtok(buf, "\n");
  while (line) {
    if (strncmp(line, "workspace>>", 11) == 0) {
      char *ws_info = line + 11;
      int wsid = atoi(ws_info);
      if (wsid < MAX_WORKSPACES && wsid >= 1) {
        // printf("id:%d\n", wsid);
        ws_array[wsid - 1].focused = true;
      }
      workspace_evt_found = 1;
    }
    line = strtok(NULL, "\n");
  }
  return workspace_evt_found ? 0 : 1;
}

static int tiny__parse_workspaces(const char *buf, HyprWorkspace workspaces[],
                                  int max_ws) {
  if (!buf || !workspaces)
    return 0;

  // Reset the entire array
  for (int i = 0; i < max_ws; i++) {
    memset(&workspaces[i], 0, sizeof(HyprWorkspace));
  }

  char *copy = strdup(buf);
  if (!copy)
    return 0;

  char *line = strtok(copy, "\n");
  int max_id_seen = 0;

  while (line) {
    // Start of a workspace block
    if (strncmp(line, "workspace ID ", 13) == 0) {
      int real_id;
      char monitor[32];

      // Parse: workspace ID X (X) on monitor MONITOR:
      if (sscanf(line, "workspace ID %d (%*d) on monitor %31s:", &real_id,
                 monitor) == 2) {
        if (real_id <= 0 || real_id > max_ws) {
          line = strtok(NULL, "\n");
          continue; // skip invalid IDs
        }

        HyprWorkspace *ws = &workspaces[real_id - 1]; // ID 1 → index 0
        memset(ws, 0, sizeof(HyprWorkspace));

        ws->id = real_id;
        strncpy(ws->monitor, monitor, sizeof(ws->monitor) - 1);
        ws->monitor[sizeof(ws->monitor) - 1] = '\0';

        if (real_id > max_id_seen)
          max_id_seen = real_id;
      }
    } else {
      // parse key-value lines
      // find the last parsed workspace
      int idx = max_id_seen - 1;
      if (idx >= 0 && idx < max_ws) {
        HyprWorkspace *ws = &workspaces[idx];

        if (sscanf(line, "        monitorID: %d", &ws->monitor_id) == 1) {
        } else if (sscanf(line, "        windows: %d", &ws->windows) == 1) {
        } else if (sscanf(line, "        hasfullscreen: %d",
                          &ws->hasfullscreen) == 1) {
        } else if (sscanf(line, "        lastwindow: %lx", &ws->lastwindow) ==
                   1) {
        } else if (sscanf(line, "        lastwindowtitle: %[^\n]",
                          ws->lastwindowtitle) == 1) {
        } else if (sscanf(line, "        ispersistent: %d",
                          &ws->ispersistent) == 1) {
        } else if (sscanf(line, "        tiledLayout: %15s", ws->tiledLayout) ==
                   1) {
        }
      }
    }

    line = strtok(NULL, "\n"); // advance line
  }

  free(copy);
  return max_id_seen; // number of workspaces parsed
}

static void tiny__workspaces_to_string(HyprWorkspace workspaces[], char *out,
                                       size_t out_size) {
  if (!workspaces || !out || out_size == 0)
    return;

  out[0] = '\0';
  for (int i = 0; i < MAX_WORKSPACES; i++) {
    if (i > 0 && !workspaces[i].focused && !workspaces[i - 1].focused)
      strncat(out, " ", out_size - strlen(out) - 1);
    else if (i == 0 && !workspaces[i].focused)
      strncat(out, " ", out_size - strlen(out) - 1);

    if (workspaces[i].focused) {
      strncat(out, "[", out_size - strlen(out) - 1);
      char tmp[16];
      snprintf(tmp, sizeof(tmp), "%d", workspaces[i].id);
      strncat(out, tmp, out_size - strlen(out) - 1);
      strncat(out, "]", out_size - strlen(out) - 1);
    } else {
      if (workspaces[i].id > 0) {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%d", workspaces[i].id);
        strncat(out, tmp, out_size - strlen(out) - 1);
      } else {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), " ");
        strncat(out, tmp, out_size - strlen(out) - 1);
      }
    }
  }
}

#endif // TINY__HYPRLAND_H
