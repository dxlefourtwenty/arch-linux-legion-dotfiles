#include "sway_window.h"

#include <errno.h>
#include <glob.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>

static long livepaper_pid(void) {
    return (long)getpid();
}

static bool g_sway_sock_cached = false;
static char g_sway_sock[PATH_MAX];

static int connect_sway_socket_path(const char *path) {
    if (!path || !*path) return -1;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    size_t n = strlen(path);
    if (n >= sizeof(addr.sun_path)) {
        close(fd);
        return -1;
    }
    memcpy(addr.sun_path, path, n + 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int connect_sway_socket(void) {
    if (g_sway_sock_cached) {
        int fd = connect_sway_socket_path(g_sway_sock);
        if (fd >= 0) return fd;
        g_sway_sock_cached = false;
        g_sway_sock[0] = '\0';
    }

    const char *env_sock = getenv("SWAYSOCK");
    if (env_sock && *env_sock) {
        int fd = connect_sway_socket_path(env_sock);
        if (fd >= 0) {
            strncpy(g_sway_sock, env_sock, sizeof(g_sway_sock) - 1);
            g_sway_sock[sizeof(g_sway_sock) - 1] = '\0';
            g_sway_sock_cached = true;
            return fd;
        }
    }

    uid_t uid = getuid();
    char pattern[PATH_MAX];
    snprintf(pattern, sizeof(pattern), "/run/user/%u/sway-ipc.%u.*.sock", (unsigned)uid, (unsigned)uid);

    glob_t matches;
    memset(&matches, 0, sizeof(matches));
    if (glob(pattern, 0, NULL, &matches) == 0) {
        for (size_t i = matches.gl_pathc; i > 0; i--) {
            const char *path = matches.gl_pathv[i - 1];
            int fd = connect_sway_socket_path(path);
            if (fd >= 0) {
                strncpy(g_sway_sock, path, sizeof(g_sway_sock) - 1);
                g_sway_sock[sizeof(g_sway_sock) - 1] = '\0';
                g_sway_sock_cached = true;
                setenv("SWAYSOCK", g_sway_sock, 1);
                globfree(&matches);
                return fd;
            }
        }
        globfree(&matches);
    }

    return -1;
}

static bool write_all(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, p + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        off += (size_t)n;
    }
    return true;
}

static bool read_all(int fd, void *buf, size_t len) {
    char *p = (char *)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, p + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        off += (size_t)n;
    }
    return true;
}

static bool sway_ipc_command(const char *payload) {
    int fd = connect_sway_socket();
    if (fd < 0) return false;

    uint32_t len = (uint32_t)strlen(payload);
    uint32_t type = 0;  // IPC_COMMAND
    char header[14];
    memcpy(header, "i3-ipc", 6);
    memcpy(header + 6, &len, 4);
    memcpy(header + 10, &type, 4);

    if (!write_all(fd, header, sizeof(header)) || !write_all(fd, payload, len)) {
        close(fd);
        return false;
    }

    char reply_header[14];
    if (!read_all(fd, reply_header, sizeof(reply_header))) {
        close(fd);
        return false;
    }

    uint32_t reply_len = 0;
    memcpy(&reply_len, reply_header + 6, 4);
    if (reply_len > 1024 * 1024) {
        close(fd);
        return false;
    }

    char *reply = (char *)malloc(reply_len + 1);
    if (!reply) {
        close(fd);
        return false;
    }
    bool ok = read_all(fd, reply, reply_len);
    close(fd);
    if (!ok) {
        free(reply);
        return false;
    }
    reply[reply_len] = '\0';

    bool success = strstr(reply, "\"success\":true") != NULL;
    free(reply);
    return success;
}

bool sway_window_is_session(void) {
    int fd = connect_sway_socket();
    if (fd < 0) return false;
    close(fd);
    return true;
}

bool sway_window_prepare_livepaper_position(int x, int y) {
    char cmd[160];
    snprintf(
        cmd,
        sizeof(cmd),
        "for_window [pid=%ld] floating enable, move position %d %d",
        livepaper_pid(),
        x,
        y);
    return sway_ipc_command(cmd);
}

bool sway_window_move_livepaper(int x, int y) {
    char cmd_pid[128];
    snprintf(
        cmd_pid,
        sizeof(cmd_pid),
        "[pid=%ld] floating enable, move position %d %d",
        livepaper_pid(),
        x,
        y);
    if (sway_ipc_command(cmd_pid)) return true;

    char cmd_app_id[128];
    snprintf(
        cmd_app_id,
        sizeof(cmd_app_id),
        "[app_id=\"livepaper\"] floating enable, move position %d %d",
        x,
        y);
    if (sway_ipc_command(cmd_app_id)) return true;

    char cmd_title[160];
    snprintf(
        cmd_title,
        sizeof(cmd_title),
        "[title=\".*livepaper.*\"] floating enable, move position %d %d",
        x,
        y);
    return sway_ipc_command(cmd_title);
}

void sway_window_focus_livepaper(void) {
    char cmd_pid[64];
    snprintf(cmd_pid, sizeof(cmd_pid), "[pid=%ld] focus", livepaper_pid());
    if (sway_ipc_command(cmd_pid)) return;
    if (sway_ipc_command("[app_id=\"livepaper\"] focus")) return;
    sway_ipc_command("[title=\".*livepaper.*\"] focus");
}
