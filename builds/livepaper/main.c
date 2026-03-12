#define _GNU_SOURCE
#include "raylib.h"
#include "sway_window.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    Color bg;
    Color idle;
    Color hover;
    Color border;
    Color overlay;
    Color text;
} Theme;

typedef struct {
    char *path;
    char *filename;
    char label[256];
    Texture2D texture;
} Wallpaper;

static Theme g_theme;
static char g_thumb_cache_dir[PATH_MAX];
static bool g_thumb_cache_ready = false;
static bool g_thumb_cache_initialized = false;

static const int UI_MAX_WALLPAPERS = 1024;
static const int UI_MAX_FPS = 144;
static const int UI_THUMB_SIZE = 148;
static const int UI_PADDING = 18;
static const int UI_GALLERY_INSET = 8;
static const int UI_LABEL_HEIGHT = 30;
static const int UI_WINDOW_MARGIN_BOTTOM = 40;
static const int UI_VISIBLE_THUMBS = 3;
static const float UI_SCROLL_FOLLOW_SPEED = 4.8f;
static const float UI_KEY_REPEAT_DELAY = 0.34f;
static const float UI_KEY_REPEAT_RATE = 0.30f;
static const float UI_APPLY_DEBOUNCE_SEC = 0.12f;

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static int wrap_index(int idx, int count) {
    int m = idx % count;
    return (m < 0) ? (m + count) : m;
}

static const char *get_home_dir(void) {
    const char *home = getenv("HOME");
    return (home && *home) ? home : ".";
}

static char *trim_whitespace(char *str) {
    if (!str) return str;
    while (*str && isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static void set_default_theme(void) {
    g_theme.bg = (Color){45, 53, 59, 255};
    g_theme.idle = (Color){52, 63, 68, 255};
    g_theme.hover = (Color){61, 72, 77, 255};
    g_theme.border = (Color){219, 188, 127, 255};
    g_theme.overlay = (Color){45, 53, 59, 220};
    g_theme.text = (Color){211, 198, 170, 255};
}

static void load_theme_config(void) {
    set_default_theme();

    char config_path[PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s/.config/hellpaper/hellpaper.conf", get_home_dir());

    FILE *f = fopen(config_path, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == ';' || line[0] == '\n') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = trim_whitespace(line);
        char *value = trim_whitespace(eq + 1);

        if (strcmp(key, "bg") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &g_theme.bg.r, &g_theme.bg.g, &g_theme.bg.b, &g_theme.bg.a);
        else if (strcmp(key, "idle") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &g_theme.idle.r, &g_theme.idle.g, &g_theme.idle.b, &g_theme.idle.a);
        else if (strcmp(key, "hover") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &g_theme.hover.r, &g_theme.hover.g, &g_theme.hover.b, &g_theme.hover.a);
        else if (strcmp(key, "border") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &g_theme.border.r, &g_theme.border.g, &g_theme.border.b, &g_theme.border.a);
        else if (strcmp(key, "overlay") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &g_theme.overlay.r, &g_theme.overlay.g, &g_theme.overlay.b, &g_theme.overlay.a);
        else if (strcmp(key, "text") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &g_theme.text.r, &g_theme.text.g, &g_theme.text.b, &g_theme.text.a);
    }

    fclose(f);
}

static void format_display_name(const char *filename, char *out, size_t out_size) {
    if (!filename || !out || out_size == 0) return;

    char temp[512];
    strncpy(temp, filename, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *dot = strrchr(temp, '.');
    if (dot) *dot = '\0';

    char *start = temp;
    while (*start && isdigit((unsigned char)*start)) start++;
    if (*start == '-') start++;

    for (char *p = start; *p; p++) {
        if (*p == '-') *p = ' ';
    }

    size_t n = strlen(start);
    if (n >= out_size) n = out_size - 1;
    memcpy(out, start, n);
    out[n] = '\0';
}

static int leading_number(const char *s) {
    int n = 0;
    bool has = false;
    while (*s && isdigit((unsigned char)*s)) {
        has = true;
        n = n * 10 + (*s - '0');
        s++;
    }
    return has ? n : -1;
}

static int cmp_wallpaper_name(const void *a, const void *b) {
    const Wallpaper *wa = (const Wallpaper *)a;
    const Wallpaper *wb = (const Wallpaper *)b;

    int na = leading_number(wa->filename);
    int nb = leading_number(wb->filename);

    if (na >= 0 && nb >= 0 && na != nb) return na - nb;
    if (na >= 0 && nb < 0) return -1;
    if (na < 0 && nb >= 0) return 1;
    return strcasecmp(wa->filename, wb->filename);
}

static bool ensure_dir(const char *path) {
    return (mkdir(path, 0755) == 0 || errno == EEXIST);
}

static bool init_thumb_cache_dir(char *out, size_t out_size) {
    char cache_root[PATH_MAX];
    char cache_app[PATH_MAX];

    if (snprintf(cache_root, sizeof(cache_root), "%s/.cache", get_home_dir()) >= (int)sizeof(cache_root)) return false;
    if (snprintf(cache_app, sizeof(cache_app), "%s/.cache/livepaper", get_home_dir()) >= (int)sizeof(cache_app)) return false;
    if (snprintf(out, out_size, "%s/thumbs", cache_app) >= (int)out_size) return false;

    if (!ensure_dir(cache_root)) return false;
    if (!ensure_dir(cache_app)) return false;
    if (!ensure_dir(out)) return false;
    return true;
}

static unsigned long long hash_fnv1a64(const char *s) {
    unsigned long long h = 1469598103934665603ULL;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ULL;
    }
    return h;
}

static bool build_thumb_cache_path(const char *img_path, const char *cache_dir, char *out, size_t out_size) {
    struct stat st;
    if (stat(img_path, &st) != 0) return false;

    char key[PATH_MAX * 2];
    if (snprintf(key, sizeof(key), "%s|%lld|%lld|%lld|%d",
                 img_path,
                 (long long)st.st_size,
                 (long long)st.st_mtim.tv_sec,
                 (long long)st.st_mtim.tv_nsec,
                 UI_THUMB_SIZE) >= (int)sizeof(key)) return false;

    unsigned long long hash = hash_fnv1a64(key);
    if (snprintf(out, out_size, "%s/%016llx.png", cache_dir, hash) >= (int)out_size) return false;
    return true;
}

static Image load_thumbnail_image(const char *img_path, const char *cache_dir) {
    char cache_path[PATH_MAX];
    bool has_cache = cache_dir && *cache_dir && build_thumb_cache_path(img_path, cache_dir, cache_path, sizeof(cache_path));

    if (has_cache && access(cache_path, F_OK) == 0) {
        Image cached = LoadImage(cache_path);
        if (cached.data && cached.width == UI_THUMB_SIZE && cached.height == UI_THUMB_SIZE) {
            return cached;
        }
        if (cached.data) UnloadImage(cached);
    }

    Image img = LoadImage(img_path);
    if (!img.data) return img;

    const int max_dim = 2048;
    if (img.width > max_dim || img.height > max_dim) {
        float scale = (img.width > img.height) ? ((float)max_dim / img.width) : ((float)max_dim / img.height);
        int nw = (int)(img.width * scale);
        int nh = (int)(img.height * scale);
        ImageResize(&img, nw, nh);
    }

    int side = (img.width < img.height) ? img.width : img.height;
    Rectangle crop = {
        (float)(img.width - side) / 2.0f,
        (float)(img.height - side) / 2.0f,
        (float)side,
        (float)side
    };

    ImageCrop(&img, crop);
    ImageResize(&img, UI_THUMB_SIZE, UI_THUMB_SIZE);

    if (has_cache) ExportImage(img, cache_path);
    return img;
}

static int collect_wallpapers(const char *dir, Wallpaper **out) {
    DIR *dp = opendir(dir);
    if (!dp) return 0;

    Wallpaper *items = calloc((size_t)UI_MAX_WALLPAPERS, sizeof(Wallpaper));
    if (!items) {
        closedir(dp);
        return 0;
    }

    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dp)) != NULL && count < UI_MAX_WALLPAPERS) {
        if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) continue;

        const char *ext = strrchr(entry->d_name, '.');
        if (!ext) continue;
        if (strcasecmp(ext, ".png") != 0 && strcasecmp(ext, ".jpg") != 0 && strcasecmp(ext, ".jpeg") != 0) continue;

        char full[PATH_MAX];
        if (snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name) >= (int)sizeof(full)) continue;

        items[count].path = strdup(full);
        items[count].filename = strdup(entry->d_name);
        format_display_name(entry->d_name, items[count].label, sizeof(items[count].label));

        items[count].texture.id = 0;

        count++;
    }

    closedir(dp);
    qsort(items, (size_t)count, sizeof(Wallpaper), cmp_wallpaper_name);

    *out = items;
    return count;
}

static void free_wallpapers(Wallpaper *items, int count) {
    if (!items) return;

    for (int i = 0; i < count; i++) {
        if (items[i].texture.id != 0) UnloadTexture(items[i].texture);
        free(items[i].path);
        free(items[i].filename);
    }
    free(items);
}

static void ensure_thumbnail_texture(Wallpaper *item) {
    if (!item || item->texture.id != 0) return;

    if (!g_thumb_cache_initialized) {
        g_thumb_cache_ready = init_thumb_cache_dir(g_thumb_cache_dir, sizeof(g_thumb_cache_dir));
        g_thumb_cache_initialized = true;
    }

    Image thumb = load_thumbnail_image(item->path, g_thumb_cache_ready ? g_thumb_cache_dir : NULL);
    if (!thumb.data) return;

    item->texture = LoadTextureFromImage(thumb);
    UnloadImage(thumb);
}

static char *shell_quote(const char *s) {
    if (!s) return strdup("''");

    size_t len = 2;
    for (const char *p = s; *p; p++) {
        len += (*p == '\'') ? 4 : 1;
    }

    char *out = malloc(len + 1);
    if (!out) return NULL;

    char *w = out;
    *w++ = '\'';
    for (const char *p = s; *p; p++) {
        if (*p == '\'') {
            memcpy(w, "'\\''", 4);
            w += 4;
        } else {
            *w++ = *p;
        }
    }
    *w++ = '\'';
    *w = '\0';

    return out;
}

static char *build_switch_command(const char *tmpl, const char *path) {
    char *quoted = shell_quote(path);
    if (!quoted) return NULL;

    const char *needle = "{}";
    size_t out_cap = strlen(tmpl) + strlen(quoted) + 8;
    char *out = malloc(out_cap);
    if (!out) {
        free(quoted);
        return NULL;
    }
    out[0] = '\0';

    const char *p = tmpl;
    bool replaced = false;
    while (*p) {
        const char *hit = strstr(p, needle);
        if (!hit) {
            size_t add = strlen(p);
            size_t cur = strlen(out);
            if (cur + add + 1 > out_cap) {
                out_cap = cur + add + 64;
                out = realloc(out, out_cap);
                if (!out) {
                    free(quoted);
                    return NULL;
                }
            }
            strcat(out, p);
            break;
        }

        size_t left = (size_t)(hit - p);
        size_t cur = strlen(out);
        size_t need = cur + left + strlen(quoted) + 1;
        if (need > out_cap) {
            out_cap = need + 64;
            out = realloc(out, out_cap);
            if (!out) {
                free(quoted);
                return NULL;
            }
        }

        strncat(out, p, left);
        strcat(out, quoted);
        replaced = true;
        p = hit + 2;
    }

    if (!replaced) {
        size_t cur = strlen(out);
        size_t need = cur + 1 + strlen(quoted) + 1;
        if (need > out_cap) {
            out = realloc(out, need + 16);
            if (!out) {
                free(quoted);
                return NULL;
            }
        }
        out[cur] = ' ';
        out[cur + 1] = '\0';
        strcat(out, quoted);
    }

    free(quoted);
    return out;
}

static bool command_uses_swww(const char *cmd) {
    return cmd && strstr(cmd, "swww") != NULL;
}

static void ensure_swww_daemon(void) {
    system("pgrep -x swww-daemon >/dev/null 2>&1 || swww-daemon >/dev/null 2>&1 &");
}

static void update_current_symlink(const char *link_path, const char *target) {
    if (!link_path || !target) return;
    unlink(link_path);
    symlink(target, link_path);
}

static void apply_wallpaper(const char *path, const char *switch_cmd, const char *link_path, bool update_link) {
    if (!path || !switch_cmd) return;

    char *cmd = build_switch_command(switch_cmd, path);
    if (!cmd) return;

    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }
    free(cmd);

    if (update_link && link_path) update_current_symlink(link_path, path);

    printf("%s\n", path);
    fflush(stdout);
}

static int find_current_index(Wallpaper *items, int count, const char *link_path) {
    char resolved[PATH_MAX];
    if (!items || count <= 0 || !link_path) return 0;
    if (!realpath(link_path, resolved)) return 0;

    for (int i = 0; i < count; i++) {
        char real_item[PATH_MAX];
        if (!realpath(items[i].path, real_item)) continue;
        if (strcmp(real_item, resolved) == 0) return i;
    }
    return 0;
}

static void print_help(void) {
    printf("livepaper - live wallpaper chooser\n\n");
    printf("Usage: livepaper [OPTIONS] [DIR]\n\n");
    printf("Options:\n");
    printf("  --switch-cmd <cmd>   Wallpaper command template (use {} for image path).\n");
    printf("  --link <path>        Symlink to update on selection change.\n");
    printf("  --no-link            Do not update wallpaper symlink.\n");
    printf("  --help               Show this help.\n\n");
    printf("Theme colors are read from ~/.config/hellpaper/hellpaper.conf on each launch.\n");
}

int main(int argc, char **argv) {
    signal(SIGCHLD, SIG_IGN);
    load_theme_config();

    char default_dir[PATH_MAX];
    char default_link[PATH_MAX];
    snprintf(default_dir, sizeof(default_dir), "%s/.config/themes/current/wallpapers", get_home_dir());
    snprintf(default_link, sizeof(default_link), "%s/.config/themes/current/wallpaper.png", get_home_dir());

    const char *wall_dir = default_dir;
    const char *link_path = default_link;
    const char *switch_cmd = getenv("LIVEPAPER_SWITCH_CMD");
    bool update_link = true;

    if (!switch_cmd || !*switch_cmd) {
        switch_cmd = "swww img --transition-type=grow --transition-duration=1.6 {}";
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
        if (strcmp(argv[i], "--switch-cmd") == 0 && i + 1 < argc) {
            switch_cmd = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--link") == 0 && i + 1 < argc) {
            link_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--no-link") == 0) {
            update_link = false;
            continue;
        }
        if (argv[i][0] != '-') {
            wall_dir = argv[i];
            continue;
        }
    }

    int window_h = UI_THUMB_SIZE + UI_LABEL_HEIGHT + UI_PADDING * 2 + 12;
    int window_w = 1200;

    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_HIDDEN);
    InitWindow(window_w, window_h, "livepaper");
    SetWindowTitle("livepaper");
    SetExitKey(KEY_NULL);
    SetTargetFPS(UI_MAX_FPS);

    int monitor = GetCurrentMonitor();
    int mon_w = GetMonitorWidth(monitor);
    int mon_h = GetMonitorHeight(monitor);

    window_w = UI_VISIBLE_THUMBS * UI_THUMB_SIZE +
               (UI_VISIBLE_THUMBS - 1) * UI_PADDING +
               (UI_PADDING + UI_GALLERY_INSET) * 2;
    if (window_w > mon_w) window_w = mon_w;

    SetWindowSize(window_w, window_h);

    int target_x = (mon_w - window_w) / 2;
    int target_y = mon_h - window_h - UI_WINDOW_MARGIN_BOTTOM;
    bool use_sway_move = sway_window_is_session();
    float sway_focus_retry_timer = 0.0f;
    bool sway_revealed = !use_sway_move;
    float row_intro_offset = use_sway_move ? 34.0f : 0.0f;

    SetWindowPosition(target_x, target_y);

    Wallpaper *items = NULL;
    int count = collect_wallpapers(wall_dir, &items);
    if (count <= 0) {
        fprintf(stderr, "livepaper: no wallpapers found in %s\n", wall_dir);
        CloseWindow();
        return 1;
    }

    if (command_uses_swww(switch_cmd)) ensure_swww_daemon();

    char font_path[PATH_MAX];
    snprintf(font_path, sizeof(font_path), "%s/.local/share/fonts/JetBrainsMonoNerdFont.ttf", get_home_dir());
    Font font = FileExists(font_path) ? LoadFontEx(font_path, 21, NULL, 0) : GetFontDefault();

    int selected_virtual = find_current_index(items, count, link_path);
    int last_applied = -1;
    int prev_selected_virtual = selected_virtual;
    float selection_idle = 0.0f;

    float step = (float)(UI_THUMB_SIZE + UI_PADDING);
    int initial_gallery_x = UI_PADDING + UI_GALLERY_INSET;
    float initial_selected_center = (float)initial_gallery_x + selected_virtual * step + UI_THUMB_SIZE * 0.5f;
    float scroll_x = initial_selected_center - window_w * 0.5f;
    float hold_timer = 0.0f;
    int hold_dir = 0;

    if (use_sway_move) {
        SetWindowOpacity(0.0f);
        sway_window_prepare_livepaper_position(target_x, target_y);
        for (int i = 0; i < 40 && !sway_revealed; i++) {
            if (sway_window_move_livepaper(target_x, target_y)) {
                sway_revealed = true;
                break;
            }
            usleep(10 * 1000);
        }
    }
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    if (use_sway_move) SetWindowOpacity(sway_revealed ? 1.0f : 0.0f);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        if (sw <= 0) sw = window_w;
        if (sh <= 0) sh = window_h;
        int gallery_x = UI_PADDING + UI_GALLERY_INSET;
        int gallery_w = sw - (UI_PADDING + UI_GALLERY_INSET) * 2;
        if (gallery_w < 1) gallery_w = 1;

        if (use_sway_move) {
            row_intro_offset = lerpf(row_intro_offset, 0.0f, dt * 10.0f);
            SetWindowOpacity(sway_revealed ? 1.0f : 0.0f);
            if (!sway_revealed && sway_window_move_livepaper(target_x, target_y)) {
                SetWindowOpacity(1.0f);
                sway_revealed = true;
            }
            if (!IsWindowFocused()) {
                sway_focus_retry_timer += dt;
                if (sway_focus_retry_timer >= 0.12f) {
                    sway_window_focus_livepaper();
                    sway_focus_retry_timer = 0.0f;
                }
            }
        } else {
            SetWindowPosition(target_x, target_y);
        }

        int row_y = sh - UI_THUMB_SIZE - UI_LABEL_HEIGHT - UI_PADDING + (int)roundf(row_intro_offset);
        int move = 0;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_L)) move = 1;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_H)) move = -1;

        int down_dir = 0;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_L)) down_dir = 1;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_H)) down_dir = -1;

        if (down_dir != 0) {
            if (down_dir != hold_dir) {
                hold_dir = down_dir;
                hold_timer = UI_KEY_REPEAT_DELAY;
            } else {
                hold_timer -= dt;
                if (hold_timer <= 0.0f) {
                    move = down_dir;
                    hold_timer = UI_KEY_REPEAT_RATE;
                }
            }
        } else {
            hold_dir = 0;
            hold_timer = 0.0f;
        }

        float wheel = GetMouseWheelMove();
        if (wheel > 0.0f) move = -1;
        if (wheel < 0.0f) move = 1;

        if (move != 0) {
            selected_virtual += move;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            int first_slot = (int)floorf((scroll_x - (float)gallery_x - UI_THUMB_SIZE) / step);
            int last_slot = (int)ceilf((scroll_x - (float)gallery_x + (float)gallery_w + UI_THUMB_SIZE) / step);
            for (int slot = first_slot; slot <= last_slot; slot++) {
                float x = (float)gallery_x + slot * step - scroll_x;
                Rectangle r = {x, (float)row_y, (float)UI_THUMB_SIZE, (float)UI_THUMB_SIZE};
                if (CheckCollisionPointRec(mouse, r) && IsWindowFocused()) {
                    selected_virtual = slot;
                    break;
                }
            }
        }

        int selected = wrap_index(selected_virtual, count);

        float selected_center = (float)gallery_x + selected_virtual * step + UI_THUMB_SIZE * 0.5f;
        float target_scroll = selected_center - sw * 0.5f;
        scroll_x = lerpf(scroll_x, target_scroll, dt * UI_SCROLL_FOLLOW_SPEED);

        if (selected_virtual != prev_selected_virtual) {
            selection_idle = 0.0f;
            prev_selected_virtual = selected_virtual;
        } else {
            selection_idle += dt;
        }

        if (selected != last_applied && selection_idle >= UI_APPLY_DEBOUNCE_SEC) {
            apply_wallpaper(items[selected].path, switch_cmd, link_path, update_link);
            last_applied = selected;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            break;
        }

        BeginDrawing();
        ClearBackground(g_theme.bg);

        DrawRectangle(0, 0, sw, sh, g_theme.overlay);
        DrawRectangleLinesEx((Rectangle){0, 0, (float)sw, (float)sh}, 2.0f, Fade(g_theme.border, 0.85f));

        BeginScissorMode(gallery_x, UI_PADDING, gallery_w, sh - UI_PADDING * 2);
        int first_slot = (int)floorf((scroll_x - (float)gallery_x - UI_THUMB_SIZE) / step);
        int last_slot = (int)ceilf((scroll_x - (float)gallery_x + (float)gallery_w + UI_THUMB_SIZE) / step);
        for (int slot = first_slot; slot <= last_slot; slot++) {
            int i = wrap_index(slot, count);
            float x = (float)gallery_x + slot * step - scroll_x;
            Rectangle rect = {x, (float)row_y, (float)UI_THUMB_SIZE, (float)UI_THUMB_SIZE};

            if (rect.x + rect.width < -4 || rect.x > sw + 4) continue;
            ensure_thumbnail_texture(&items[i]);

            bool is_selected = (slot == selected_virtual);
            Color cell = is_selected ? g_theme.hover : g_theme.idle;
            DrawRectangleRounded(rect, 0.08f, 8, cell);

            if (items[i].texture.id != 0) {
                DrawTexturePro(
                    items[i].texture,
                    (Rectangle){0, 0, (float)items[i].texture.width, (float)items[i].texture.height},
                    rect,
                    (Vector2){0, 0},
                    0.0f,
                    WHITE
                );
            }

            DrawRectangleLinesEx(rect, is_selected ? 3.0f : 1.0f, is_selected ? g_theme.border : Fade(g_theme.border, 0.35f));

            float font_size = 17.0f;
            float spacing = 1.0f;
            Vector2 label_size = MeasureTextEx(font, items[i].label, font_size, spacing);
            float tx = rect.x + (rect.width - label_size.x) * 0.5f;
            float ty = rect.y + rect.height + 7.0f;
            DrawTextEx(font, items[i].label, (Vector2){tx, ty}, font_size, spacing, g_theme.text);
        }
        EndScissorMode();

        EndDrawing();
    }

    if (font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id) {
        UnloadFont(font);
    }

    CloseWindow();
    free_wallpapers(items, count);
    return 0;
}
