#define _GNU_SOURCE
#include "hellpaper.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char g_thumb_cache_dir[1024];
static bool g_thumb_cache_ready = false;

static bool EnsureDirectory(const char *path)
{
    if (mkdir(path, 0755) == 0 || errno == EEXIST)
    {
        return true;
    }
    LogMessage(LOG_WARNING, "Could not create cache directory '%s': %s", path, strerror(errno));
    return false;
}

static void InitThumbnailCache(void)
{
    const char *home = get_home_dir();
    if (!home || !*home)
    {
        return;
    }

    char cache_root[1024];
    char app_cache[1024];
    if (snprintf(cache_root, sizeof(cache_root), "%s/.cache", home) >= (int)sizeof(cache_root)) return;
    if (snprintf(app_cache, sizeof(app_cache), "%s/.cache/hellpaper", home) >= (int)sizeof(app_cache)) return;
    if (snprintf(g_thumb_cache_dir, sizeof(g_thumb_cache_dir), "%s/thumbs", app_cache) >= (int)sizeof(g_thumb_cache_dir)) return;

    if (!EnsureDirectory(cache_root)) return;
    if (!EnsureDirectory(app_cache)) return;
    if (!EnsureDirectory(g_thumb_cache_dir)) return;

    g_thumb_cache_ready = true;
}

static unsigned long long HashFNV1a64(const char *s)
{
    unsigned long long h = 1469598103934665603ULL;
    while (*s)
    {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ULL;
    }
    return h;
}

static bool BuildThumbnailCachePath(const char *wallpaper_path, char *out, size_t out_size)
{
    if (!g_thumb_cache_ready || !wallpaper_path || !out || out_size == 0)
    {
        return false;
    }

    struct stat st;
    if (stat(wallpaper_path, &st) != 0)
    {
        return false;
    }

    char key[2048];
    snprintf(
        key,
        sizeof(key),
        "%s|%lld|%lld|%lld|%d",
        wallpaper_path,
        (long long)st.st_size,
        (long long)st.st_mtim.tv_sec,
        (long long)st.st_mtim.tv_nsec,
        g_base_thumb_size
    );

    unsigned long long hash = HashFNV1a64(key);
    snprintf(out, out_size, "%s/%016llx.png", g_thumb_cache_dir, hash);
    return true;
}

static Image LoadThumbnailImage(const char *fp)
{
    char cache_path[1200];
    bool has_cache_path = BuildThumbnailCachePath(fp, cache_path, sizeof(cache_path));

    if (has_cache_path && access(cache_path, F_OK) == 0)
    {
        Image cached = LoadImage(cache_path);
        if (cached.data && cached.width == g_base_thumb_size && cached.height == g_base_thumb_size)
        {
            return cached;
        }
        if (cached.data)
        {
            UnloadImage(cached);
        }
    }

    LogMessage(LOG_INFO, "LoadImage: %s", fp);
    Image i = LoadImage(fp);

    if (!i.data)
    {
        LogMessage(LOG_WARNING, "Failed to load: %s", fp);
        return i;
    }

    LogMessage(LOG_INFO, "Loaded %s (%dx%d)", fp, i.width, i.height);

    const int MAX_SAFE_DIM = 2048;

    if (i.width > MAX_SAFE_DIM || i.height > MAX_SAFE_DIM)
    {
        float scale = 1.0f;

        if (i.width > i.height)
            scale = (float)MAX_SAFE_DIM / (float)i.width;
        else
            scale = (float)MAX_SAFE_DIM / (float)i.height;

        int newW = (int)(i.width * scale);
        int newH = (int)(i.height * scale);

        LogMessage(LOG_INFO, "Downscaling %s to %dx%d", fp, newW, newH);
        ImageResize(&i, newW, newH);
    }

    int cs = (i.width < i.height) ? i.width : i.height;

    Rectangle cr = {
        (float)(i.width - cs) / 2,
        (float)(i.height - cs) / 2,
        (float)cs,
        (float)cs
    };

    LogMessage(LOG_INFO, "Crop: %s", fp);
    ImageCrop(&i, cr);

    LogMessage(LOG_INFO, "Final resize: %s", fp);
    ImageResize(&i, g_base_thumb_size, g_base_thumb_size);

    if (has_cache_path)
    {
        ExportImage(i, cache_path);
    }

    LogMessage(LOG_INFO, "Thumbnail OK: %s", fp);
    return i;
}

void *FullPreviewLoaderThread(void* p)
{
    const char* path = (const char*)p;

    LogMessage(LOG_INFO, "Loading preview: %s", path);

    Image i = LoadImage(path);

    if (i.data)
    {
        pendingFullImage = i;
        atomic_store(&fullImagePending, true);
    }
    return NULL;
}

void *LoaderThread(void* a)
{
    (void)a;

    while (atomic_load(&loader_running))
    {
        int i = atomic_fetch_add(&next_load_index, 1);
        if (i >= wallpaper_count)
        {
            usleep(100000);
            continue;
        }
        if (!atomic_load(&imagePending[i]) && !atomic_load(&wallpapers[i].loaded))
        {
            LogMessage(LOG_INFO, "Loading thumbnail: %s", wallpapers[i].path);

            Image img = LoadThumbnailImage(wallpapers[i].path);
            if (img.data)
            {
                pendingImages[i] = img;
                atomic_store(&imagePending[i], true);
            }
        }
    }
    return NULL;
}

static int ExtractLeadingNumber(const char *filename)
{
    int number = 0;
    while (*filename && isdigit((unsigned char)*filename))
    {
        number = number * 10 + (*filename - '0');
        filename++;
    }
    return number;
}

static int CompareWallpapers(const void *a, const void *b)
{
    const Wallpaper *wa = (const Wallpaper *)a;
    const Wallpaper *wb = (const Wallpaper *)b;

    int na = ExtractLeadingNumber(wa->filename);
    int nb = ExtractLeadingNumber(wb->filename);

    if (na != 0 || nb != 0)
        return na - nb;

    return strcasecmp(wa->filename, wb->filename);
}

void LoadWallpapers(const char *dir)
{
    InitThumbnailCache();

    DIR *dp = opendir(dir);
    if (!dp)
    {
        LogMessage(LOG_ERROR, "Could not open wallpaper directory: %s", dir);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL && wallpaper_count < g_max_wallpapers)
    {
        if (entry->d_type != DT_REG)
        {
            continue;
        }
        const char *ext = strrchr(entry->d_name, '.');
        if (!ext || (strcasecmp(ext, ".jpg") != 0 && strcasecmp(ext, ".jpeg") != 0 && strcasecmp(ext, ".png") != 0))
        {
            continue;
        }

        char *fullpath;
        if (asprintf(&fullpath, "%s/%s", dir, entry->d_name) == -1) continue;

        wallpapers[wallpaper_count] = (Wallpaper){
            .path = strdup(fullpath),
            .filename = strdup(entry->d_name),
            .loaded = false,
            .hoverAnim = 0.0f,
            .animPos = {(float)GetRandomValue(-500, 500), (float)GetRandomValue(800, 1200)},
            .animSize = {g_base_thumb_size, g_base_thumb_size}
        };
        free(fullpath);
        atomic_store(&imagePending[wallpaper_count], false);
        wallpaper_count++;
    }
    closedir(dp);

    qsort(wallpapers, wallpaper_count, sizeof(Wallpaper), CompareWallpapers);

}
