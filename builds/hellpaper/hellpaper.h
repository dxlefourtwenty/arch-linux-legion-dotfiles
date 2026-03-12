#ifndef HELLPAPER_H
#define HELLPAPER_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    Color bg;
    Color idle;
    Color hover;
    Color border;
    Color ripple;
    Color overlay;
    Color text;
    Color searchBg;
    Color searchBorder;
    Color searchText;
    Color searchHint;
    Color searchCount;
} Theme;

extern Theme AppTheme;

#define MAX_POSSIBLE_WALLPAPERS 2048
#define MAX_POSSIBLE_PARTICLES 256
#define MAX_TEXTURES_TO_LOAD_PER_FRAME 4

typedef enum {
    MODE_GRID = 0,
    MODE_RIVER_H,
    MODE_RIVER_V,
    MODE_WAVE
} LayoutMode;

#define NUM_MODES 4

typedef struct {
    float hoverAnim;
    atomic_bool loaded;
    char *path, *filename;

    Texture2D texture;
    Vector2 animPos, animSize;
} Wallpaper;

typedef struct
{
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
} Particle;

typedef enum
{
    EFFECT_NONE,
    EFFECT_GLITCH,
    EFFECT_BLUR,
    EFFECT_PIXELATE,
    EFFECT_REVEAL,
    EFFECT_SHAKE,
    EFFECT_COLLAPSE
} EffectType;

extern const char *postProcessingFs;
extern const char *blurFs;

extern Wallpaper wallpapers[MAX_POSSIBLE_WALLPAPERS];
extern Particle particles[MAX_POSSIBLE_PARTICLES];

extern int g_base_thumb_size;
extern int g_max_wallpapers;
extern int g_particle_count;
extern int g_base_padding;
extern int g_max_threads;
extern int g_max_fps;
extern float g_anim_speed;
extern float g_ken_burns_duration;
extern float g_border_thickness_bloom;

extern int wallpaper_count;
extern bool print_filename_only;

extern atomic_int next_load_index;
extern atomic_bool loader_running;

extern Image pendingImages[MAX_POSSIBLE_WALLPAPERS];
extern atomic_bool imagePending[MAX_POSSIBLE_WALLPAPERS];

extern int preview_index;
extern float previewAnim;
extern int closing_preview_index;

extern Texture2D fullPreviewTexture;
extern atomic_bool isFullTextureReady;
extern Image pendingFullImage;
extern atomic_bool fullImagePending;

extern bool isExiting;
extern float masterScale;

extern char searchBuffer[256];
extern int searchBufferCount;
extern bool isSearching;

extern float kenBurnsTimer;

extern EffectType g_startupEffect;
extern EffectType g_keypressEffect;
extern EffectType g_exitEffect;

extern float g_effectIntensity;
extern float g_effectTimer;
extern float g_effectDuration;
extern EffectType g_activeEffect;

extern LayoutMode g_currentMode;
extern LayoutMode g_targetMode;
extern float g_modeTransitionTimer;
extern const float g_modeTransitionDuration;

extern Vector2 g_scroll;
extern float g_smoothScrollY;
extern float g_smoothScrollX;
extern float g_topInset;

extern int g_hoveredIndex;
extern Rectangle g_previewStartRect;

extern int g_win_width;
extern int g_win_height;

extern Font gFont;

int GetGridColumnCount(float screenWidth, float thumbSize, float padding);
float GetGridStartX(float screenWidth, int columns, float thumbSize, float padding);
void CenterWindowOnCurrentMonitor(void);

void LogMessage(int level, const char *format, ...);
const char *get_home_dir(void);
const char* stristr(const char* haystack, const char* needle);
char* trim_whitespace(char* str);
void FormatDisplayName(const char *filename, char *out, size_t outSize);

void TriggerParticleBurst(Vector2 pos);
void TriggerEffect(EffectType type, float duration);
EffectType ParseEffect(const char* arg);

void *FullPreviewLoaderThread(void *p);
void *LoaderThread(void *a);
void LoadWallpapers(const char *dir);

void LoadDefaultConfig(void);
void ParseConfigFile(void);
void print_help(void);

void UpdateAndDrawScene(int filteredCount, int* filteredIndices, float delta, bool isPreviewing, bool isSearching);

#endif
