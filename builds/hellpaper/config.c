#include "hellpaper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void LoadDefaultConfig(void)
{
    AppTheme.bg = (Color){10, 10, 15, 255};
    AppTheme.idle = (Color){30, 30, 46, 255};
    AppTheme.hover = (Color){49, 50, 68, 255};
    AppTheme.border = (Color){203, 166, 247, 255};
    AppTheme.ripple = (Color){245, 194, 231, 255};
    AppTheme.overlay = (Color){10, 10, 15, 200};
    AppTheme.text = (Color){202, 212, 241, 255};
    AppTheme.searchBg = AppTheme.overlay;
    AppTheme.searchBorder = AppTheme.border;
    AppTheme.searchText = AppTheme.text;
    AppTheme.searchHint = Fade(AppTheme.text, 0.65f);
    AppTheme.searchCount = AppTheme.ripple;

    g_startupEffect = EFFECT_NONE;
    g_keypressEffect = EFFECT_NONE;
    g_exitEffect = EFFECT_NONE;

    g_max_wallpapers = 512;
    g_base_thumb_size = 150;
    g_base_padding = 15;
    g_border_thickness_bloom = 3.0f;

    g_max_threads = 1;
    g_anim_speed = 30.0f;
    g_particle_count = 50;
    g_ken_burns_duration = 15.0f;
    g_max_fps = 200;

    g_win_width = 1280;
    g_win_height = 720;
}

void ParseConfigFile(void)
{
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/.config/hellpaper", get_home_dir());
    mkdir(config_path, 0755);
    snprintf(config_path, sizeof(config_path), "%s/.config/hellpaper/hellpaper.conf", get_home_dir());

    FILE *file = fopen(config_path, "r");
    if (!file) {
        LogMessage(LOG_INFO, "Config file not found at '%s'. Using default settings.", config_path);
        return;
    }

    bool searchBgConfigured = false;
    bool searchBorderConfigured = false;
    bool searchTextConfigured = false;
    bool searchHintConfigured = false;
    bool searchCountConfigured = false;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == ';' || line[0] == '\n') continue;

        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n");

        if (key && value) {
            key = trim_whitespace(key);
            value = trim_whitespace(value);

            if (strcmp(key, "bg") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.bg.r, &AppTheme.bg.g, &AppTheme.bg.b, &AppTheme.bg.a);
            else if (strcmp(key, "width") == 0) g_win_width = atoi(value);
            else if (strcmp(key, "height") == 0) g_win_height = atoi(value);
            else if (strcmp(key, "idle") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.idle.r, &AppTheme.idle.g, &AppTheme.idle.b, &AppTheme.idle.a);
            else if (strcmp(key, "hover") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.hover.r, &AppTheme.hover.g, &AppTheme.hover.b, &AppTheme.hover.a);
            else if (strcmp(key, "border") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.border.r, &AppTheme.border.g, &AppTheme.border.b, &AppTheme.border.a);
            else if (strcmp(key, "ripple") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.ripple.r, &AppTheme.ripple.g, &AppTheme.ripple.b, &AppTheme.ripple.a);
            else if (strcmp(key, "overlay") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.overlay.r, &AppTheme.overlay.g, &AppTheme.overlay.b, &AppTheme.overlay.a);
            else if (strcmp(key, "text") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.text.r, &AppTheme.text.g, &AppTheme.text.b, &AppTheme.text.a);
            else if (strcmp(key, "search_bg") == 0) { sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.searchBg.r, &AppTheme.searchBg.g, &AppTheme.searchBg.b, &AppTheme.searchBg.a); searchBgConfigured = true; }
            else if (strcmp(key, "search_border") == 0) { sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.searchBorder.r, &AppTheme.searchBorder.g, &AppTheme.searchBorder.b, &AppTheme.searchBorder.a); searchBorderConfigured = true; }
            else if (strcmp(key, "search_text") == 0) { sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.searchText.r, &AppTheme.searchText.g, &AppTheme.searchText.b, &AppTheme.searchText.a); searchTextConfigured = true; }
            else if (strcmp(key, "search_hint") == 0) { sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.searchHint.r, &AppTheme.searchHint.g, &AppTheme.searchHint.b, &AppTheme.searchHint.a); searchHintConfigured = true; }
            else if (strcmp(key, "search_count") == 0) { sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.searchCount.r, &AppTheme.searchCount.g, &AppTheme.searchCount.b, &AppTheme.searchCount.a); searchCountConfigured = true; }

            else if (strcmp(key, "max_wallpapers") == 0) g_max_wallpapers = atoi(value);
            else if (strcmp(key, "base_thumb_size") == 0) g_base_thumb_size = atoi(value);
            else if (strcmp(key, "base_padding") == 0) g_base_padding = atoi(value);
            else if (strcmp(key, "border_thickness_bloom") == 0) g_border_thickness_bloom = atof(value);
            else if (strcmp(key, "max_threads") == 0) g_max_threads = atoi(value);
            else if (strcmp(key, "anim_speed") == 0) g_anim_speed = atof(value);
            else if (strcmp(key, "particle_count") == 0) g_particle_count = atoi(value);
            else if (strcmp(key, "ken_burns_duration") == 0) g_ken_burns_duration = atof(value);
            else if (strcmp(key, "max_fps") == 0) g_max_fps = atoi(value);

            else if (strcmp(key, "startup_effect") == 0) g_startupEffect = ParseEffect(value);
            else if (strcmp(key, "keypress_effect") == 0) g_keypressEffect = ParseEffect(value);
            else if (strcmp(key, "exit_effect") == 0) g_exitEffect = ParseEffect(value);
        }
    }
    fclose(file);

    if (!searchBgConfigured) AppTheme.searchBg = AppTheme.overlay;
    if (!searchBorderConfigured) AppTheme.searchBorder = AppTheme.border;
    if (!searchTextConfigured) AppTheme.searchText = AppTheme.text;
    if (!searchHintConfigured) AppTheme.searchHint = Fade(AppTheme.text, 0.65f);
    if (!searchCountConfigured) AppTheme.searchCount = AppTheme.ripple;
}

void print_help(void)
{
    printf("Hellpaper - wallpaper picker for Linux.\n\n");
    printf("USAGE:\n");
    printf("  hellpaper [OPTIONS] [PATH]\n\n");
    printf("ARGUMENTS:\n");
    printf("  [PATH]              Optional path to the directory containing wallpapers.\n");
    printf("                      Defaults to '~/Pictures/'.\n\n");
    printf("OPTIONS:\n");
    printf("  --help              Show this help message and exit.\n");
    printf("  --filename          Print only the filename of the selected wallpaper to stdout.\n");
    printf("  --width <pixels>    Set the initial window width.\n");
    printf("  --height <pixels>   Set the initial window height.\n");
    printf("  --startup-effect <effect>\n");
    printf("  --keypress-effect <effect>\n");
    printf("  --exit-effect <effect>\n");
    printf("                      Override the configured visual effects on certain events.\n");
    printf("                      Available effects: none, glitch, blur, pixelate, shake, collapse, reveal\n\n");
    printf("KEYBINDINGS:\n");
    printf("  NAVIGATION:\n");
    printf("    h, j, k, l / Arrows Move selection. Keys repeat when held.\n");
    printf("    Mouse Wheel       Scroll through wallpapers.\n");
    printf("    Ctrl + Mouse Wheel  Zoom thumbnail scaling.\n\n");
    printf("  ACTIONS:\n");
    printf("    Enter / LMB         Select the highlighted wallpaper and exit.\n");
    printf("    L-Shift / RMB       Show a full-screen preview of the highlighted wallpaper.\n");
    printf("    /                   Enter search mode. Type to filter wallpapers by name.\n");
    printf("    Ctrl + f            Focus search mode without clearing the current query.\n");
    printf("    ESC                 Closes Preview, then Search, then the App.\n\n");
    printf("  VIEW MODES:\n");
    printf("    1, 2, 3, 4          Switch between different layout modes:\n");
    printf("                        1: Grid\n");
    printf("                        2: Horizontal River\n");
    printf("                        3: Vertical River\n");
    printf("                        4: Wave\n\n");
    printf("CONFIGURATION:\n");
    printf("  Hellpaper can be fully customized by editing the configuration file located at:\n");
    printf("  ~/.config/hellpaper/hellpaper.conf\n");
    printf("  Search UI colors inherit from the base theme by default and can be overridden with:\n");
    printf("  search_bg, search_border, search_text, search_hint, search_count\n");
}
