#include "hellpaper.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int GetGridColumnCount(float screenWidth, float thumbSize, float padding)
{
    int columns = (int)(screenWidth / (thumbSize + padding));
    if (columns < 1) columns = 1;
    return columns;
}

float GetGridStartX(float screenWidth, int columns, float thumbSize, float padding)
{
    float contentWidth = columns * thumbSize + (columns - 1) * padding;
    float centeredX = (screenWidth - contentWidth) * 0.5f;
    return centeredX < padding ? padding : centeredX;
}

void CenterWindowOnCurrentMonitor(void)
{
    int monitor = GetCurrentMonitor();
    Vector2 monitorPos = GetMonitorPosition(monitor);
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    int windowWidth = GetScreenWidth();
    int windowHeight = GetScreenHeight();

    int windowX = (int)monitorPos.x + (monitorWidth - windowWidth) / 2;
    int windowY = (int)monitorPos.y + (monitorHeight - windowHeight) / 2;
    SetWindowPosition(windowX, windowY);
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            print_help();
            return 0;
        }
    }

    char *wallpaper_path = NULL;
    char default_path[1024];
    LoadDefaultConfig();
    ParseConfigFile();
    SetTraceLogLevel(LOG_ERROR);

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--startup-effect") == 0 && i + 1 < argc)
        {
            g_startupEffect = ParseEffect(argv[++i]);
        }
        else if (strcmp(argv[i], "--keypress-effect") == 0 && i + 1 < argc)
        {
            g_keypressEffect = ParseEffect(argv[++i]);
        }
        else if (strcmp(argv[i], "--exit-effect") == 0 && i + 1 < argc)
        {
            g_exitEffect = ParseEffect(argv[++i]);
        }
        else if (strcmp(argv[i], "--filename") == 0)
        {
            print_filename_only = true;
        }
        else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
        {
            g_win_width = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
        {
            g_win_height = atoi(argv[++i]);
        }
        else if (argv[i][0] != '-' && wallpaper_path == NULL)
        {
            wallpaper_path = argv[i];
        }
    }

    if (!wallpaper_path)
    {
        snprintf(default_path, sizeof(default_path), "%s/Pictures", get_home_dir());
        wallpaper_path = default_path;
    }
    LoadWallpapers(wallpaper_path);

    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(g_win_width, g_win_height, "Hellpaper");
    CenterWindowOnCurrentMonitor();
    EnableCursor();
    ShowCursor();
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    gFont = LoadFontEx("/home/dxle/.local/share/fonts/JetBrainsMonoNerdFont.ttf", 28, 0, 0);

    SetExitKey(KEY_NULL);
    SetTargetFPS(g_max_fps);

    const float bloomDownscale = 4.0f;
    Shader postShader = LoadShaderFromMemory(NULL, postProcessingFs);
    int timeLoc = GetShaderLocation(postShader, "time");
    int resLoc = GetShaderLocation(postShader, "resolution");
    int glitchLoc = GetShaderLocation(postShader, "glitchIntensity");
    int blurLoc = GetShaderLocation(postShader, "blurIntensity");
    int pixelLoc = GetShaderLocation(postShader, "pixelSize");
    int mirrorLoc = GetShaderLocation(postShader, "mirrorMode");
    int shakeLoc = GetShaderLocation(postShader, "shakeIntensity");
    int collapseLoc = GetShaderLocation(postShader, "collapseIntensity");
    int scanlineLoc = GetShaderLocation(postShader, "scanlineIntensity");

    Shader blurShader = LoadShaderFromMemory(NULL, blurFs);
    RenderTexture2D mainTarget = LoadRenderTexture(g_win_width, g_win_height);
    RenderTexture2D bloomMask = LoadRenderTexture(g_win_width / bloomDownscale, g_win_height / bloomDownscale);
    RenderTexture2D blurPingPong = LoadRenderTexture(g_win_width / bloomDownscale, g_win_height / bloomDownscale);
    RenderTexture2D bloomMaskHiRes = LoadRenderTexture(g_win_width, g_win_height);

    pthread_t loader_threads[g_max_threads];
    for (int t = 0; t < g_max_threads; t++)
    {
        pthread_create(&loader_threads[t], NULL, LoaderThread, NULL);
    }
    TriggerEffect(g_startupEffect, 1.0f);

    float keyRepeatTimer = 0.0f;
    const float KEY_REPEAT_INITIAL_DELAY = 0.25f;
    const float KEY_REPEAT_DELAY = 0.16f;
    const float SEARCH_UI_HEIGHT = 52.0f;

    while (!WindowShouldClose())
    {
        float delta = GetFrameTime();
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        if (isExiting && g_effectTimer <= 0)
        {
            break;
        }

        if (IsWindowResized())
        {
            UnloadRenderTexture(mainTarget);
            UnloadRenderTexture(bloomMask);
            UnloadRenderTexture(blurPingPong);
            UnloadRenderTexture(bloomMaskHiRes);
            mainTarget = LoadRenderTexture(sw, sh);
            bloomMask = LoadRenderTexture(sw / bloomDownscale, sh / bloomDownscale);
            blurPingPong = LoadRenderTexture(sw / bloomDownscale, sh / bloomDownscale);
            bloomMaskHiRes = LoadRenderTexture(sw, sh);
        }

        if (g_effectTimer > 0)
        {
            g_effectTimer -= delta;
            float p = 1.f - (g_effectTimer / g_effectDuration);
            if (g_activeEffect == EFFECT_COLLAPSE || g_activeEffect == EFFECT_REVEAL)
            {
                g_effectIntensity = p;
            }
            else
            {
                g_effectIntensity = sinf(p * PI);
            }
        }
        else
        {
            g_effectIntensity = 0.f;
            g_activeEffect = EFFECT_NONE;
        }

        bool isPreviewing = (preview_index != -1);

        int filteredIndices[g_max_wallpapers];
        int filteredCount = 0;
        for (int i = 0; i < wallpaper_count; i++)
        {
            if (searchBufferCount == 0 || stristr(wallpapers[i].filename, searchBuffer) != NULL)
            {
                filteredIndices[filteredCount++] = i;
            }
        }

        bool blockActions = isExiting || isPreviewing;
        if (!blockActions)
        {
            int key = GetKeyPressed();
            if (key != 0 && !isSearching)
            {
                TriggerEffect(g_keypressEffect, 0.4f);
            }
            if (key >= KEY_ONE && key < KEY_ONE + NUM_MODES)
            {
                g_targetMode = key - KEY_ONE;
                g_modeTransitionTimer = g_modeTransitionDuration;
            }
        }

        bool ateEscKey = false;
        if (isSearching)
        {
            bool resetSearchQuery = IsKeyPressed(KEY_SLASH);
            if (resetSearchQuery)
            {
                searchBufferCount = 0;
                searchBuffer[0] = '\0';
            }

            int key = GetCharPressed();
            while (key > 0)
            {
                if ((key >= 32) && (key <= 125) && (searchBufferCount < 255))
                {
                    if (!(resetSearchQuery && key == '/'))
                    {
                        searchBuffer[searchBufferCount++] = (char)key;
                    }
                }
                key = GetCharPressed();
            }
            bool ctrlHeld = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            if (IsKeyPressed(KEY_BACKSPACE))
            {
                if (ctrlHeld)
                {
                    searchBufferCount = 0;
                    searchBuffer[0] = '\0';
                }
                else if (searchBufferCount > 0)
                {
                    searchBuffer[--searchBufferCount] = '\0';
                }
            }
            if (IsKeyPressed(KEY_DELETE))
            {
                searchBufferCount = 0;
                searchBuffer[0] = '\0';
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
            {
                isSearching = false;
                if (IsKeyPressed(KEY_ESCAPE)) ateEscKey = true;
            }
        }
        else
        {
            bool ctrlHeld = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            if (!blockActions && IsKeyPressed(KEY_SLASH))
            {
                isSearching = true;
                searchBufferCount = 0;
                searchBuffer[0] = '\0';
            }
            else if (!blockActions && ctrlHeld && IsKeyPressed(KEY_F))
            {
                isSearching = true;
            }
        }

        bool searchUIVisible = isSearching || searchBufferCount > 0;
        g_topInset = searchUIVisible ? SEARCH_UI_HEIGHT : 0.0f;

        if (!blockActions && !isSearching && filteredCount > 0)
        {
            int direction = 0;
            keyRepeatTimer -= delta;

            bool navKeyDown = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_L) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_H) ||
                              IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_J) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_K);
            bool navKeyJustPressed = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_L) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_H) ||
                                     IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K);

            if (navKeyJustPressed || (navKeyDown && keyRepeatTimer <= 0.0f)) {
                keyRepeatTimer = navKeyJustPressed ? KEY_REPEAT_INITIAL_DELAY : KEY_REPEAT_DELAY;

                if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_L)) direction = 1;
                if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_H)) direction = -1;
                if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_J))
                {
                    if (g_currentMode == MODE_GRID) {
                        int cols = GetGridColumnCount(GetScreenWidth(), g_base_thumb_size * masterScale, g_base_padding * masterScale);
                        direction = cols;
                    } else direction = 1;
                }
                if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_K))
                {
                    if (g_currentMode == MODE_GRID) {
                        int cols = GetGridColumnCount(GetScreenWidth(), g_base_thumb_size * masterScale, g_base_padding * masterScale);
                        direction = -cols;
                    } else direction = -1;
                }

                if (direction != 0)
                {
                    int currentFilteredIdx = -1;
                    for (int i = 0; i < filteredCount; i++)
                    {
                        if (filteredIndices[i] == g_hoveredIndex)
                        {
                            currentFilteredIdx = i;
                            break;
                        }
                    }

                    int nextFilteredIdx = (currentFilteredIdx == -1) ? ((direction > 0) ? 0 : filteredCount - 1) : (currentFilteredIdx + direction);
                    nextFilteredIdx = Clamp(nextFilteredIdx, 0, filteredCount - 1);
                    g_hoveredIndex = filteredIndices[nextFilteredIdx];

                    Rectangle itemRect;
                    float ts = g_base_thumb_size * masterScale;
                    float p  = g_base_padding * masterScale;

                    currentFilteredIdx = nextFilteredIdx;

                    int cols = GetGridColumnCount((float)sw, ts, p);
                    float startX = GetGridStartX((float)sw, cols, ts, p);

                    int row = currentFilteredIdx / cols;
                    int col = currentFilteredIdx % cols;

                    float targetX = startX + col * (ts + p);
                    float targetY = row * (ts + p) + p;

                    itemRect = (Rectangle){
                        targetX,
                        targetY,
                        ts,
                        ts
                    };

                    if (g_currentMode == MODE_GRID)
                    {
                        if (itemRect.y < g_scroll.y)
                            g_scroll.y = itemRect.y - p;

                        if (itemRect.y + itemRect.height > g_scroll.y + sh)
                            g_scroll.y = itemRect.y + itemRect.height - sh + p;
                    }
                }
            }
            else if (!navKeyDown)
            {
                keyRepeatTimer = 0.0f;
            }
        }

        float wheel = GetMouseWheelMove();
        float maxScrollY = 0, maxScrollX = 0;
        float usableHeight = sh - g_topInset;
        if (usableHeight < 1.0f) usableHeight = 1.0f;
        switch(g_currentMode)
        {
            case MODE_GRID:
                {
                    float ts = g_base_thumb_size * masterScale, p = g_base_padding * masterScale;
                    int c = sw / (ts + p); if (c < 1) c = 1;
                    int r = (filteredCount + c - 1) / c;
                    if (r > 0) maxScrollY = r * (ts + p) - usableHeight + p;
                    break;
                }
            case MODE_RIVER_V:
                {
                    maxScrollY = filteredCount * (g_base_thumb_size * 0.7f * masterScale) - usableHeight + (g_base_thumb_size * 1.5f * masterScale);
                    break;
                }
            case MODE_RIVER_H:
            case MODE_WAVE:
                {
                    maxScrollX = filteredCount * (g_base_thumb_size * 0.8f * masterScale) - sw + (g_base_thumb_size * 1.5f * masterScale);
                    break;
                }
        }
        if (maxScrollY < 0) maxScrollY = 0;
        if (maxScrollX < 0) maxScrollX = 0;

        if (IsKeyDown(KEY_LEFT_CONTROL))
        {
            masterScale += wheel * 0.05f;
            masterScale = Clamp(masterScale, 0.2f, 5.0f);
        }
        else if (!isPreviewing)
        {
            if(g_currentMode == MODE_RIVER_H || g_currentMode == MODE_WAVE) g_scroll.x -= wheel * 100.f;
            else g_scroll.y -= wheel * 100.f;
        }

        g_scroll.y = Clamp(g_scroll.y, 0, maxScrollY);
        g_scroll.x = Clamp(g_scroll.x, 0, maxScrollX);

        if (!isPreviewing && !isExiting && !isSearching && g_hoveredIndex != -1)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER))
            {
                isExiting = true;
                TriggerParticleBurst(GetMousePosition());
                printf("%s\n", print_filename_only ? wallpapers[g_hoveredIndex].filename : wallpapers[g_hoveredIndex].path);
                fflush(stdout);
                TriggerEffect(g_exitEffect, 1.5f);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_LEFT_SHIFT))
            {
                preview_index = g_hoveredIndex;
                closing_preview_index = g_hoveredIndex;
                atomic_store(&isFullTextureReady, false);
                g_previewStartRect = (Rectangle){wallpapers[g_hoveredIndex].animPos.x, wallpapers[g_hoveredIndex].animPos.y, wallpapers[g_hoveredIndex].animSize.x, wallpapers[g_hoveredIndex].animSize.y};
                pthread_t pt;
                pthread_create(&pt, NULL, FullPreviewLoaderThread, wallpapers[g_hoveredIndex].path);
                pthread_detach(pt);
            }
        }

        if (isPreviewing && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)))
        {
            if(IsKeyPressed(KEY_ESCAPE)) ateEscKey = true;
            if (atomic_load(&isFullTextureReady)) UnloadTexture(fullPreviewTexture);
            closing_preview_index = preview_index;
            g_previewStartRect = (Rectangle){wallpapers[closing_preview_index].animPos.x, wallpapers[closing_preview_index].animPos.y, wallpapers[closing_preview_index].animSize.x, wallpapers[closing_preview_index].animSize.y};
            preview_index = -1;
        }

        if (IsKeyPressed(KEY_ESCAPE) && !ateEscKey) {
            break;
        }

        g_smoothScrollY = Lerp(g_smoothScrollY, g_scroll.y, delta * g_anim_speed);
        g_smoothScrollX = Lerp(g_smoothScrollX, g_scroll.x, delta * g_anim_speed);

        int textures_loaded_this_frame = 0;
        for (int i = 0; i < wallpaper_count; i++)
        {
            if (textures_loaded_this_frame >= MAX_TEXTURES_TO_LOAD_PER_FRAME) break;
            if (atomic_load(&imagePending[i]))
            {
                wallpapers[i].texture = LoadTextureFromImage(pendingImages[i]);
                UnloadImage(pendingImages[i]);
                atomic_store(&wallpapers[i].loaded, true);
                atomic_store(&imagePending[i], false);
                textures_loaded_this_frame++;
            }
        }

        if (atomic_load(&fullImagePending))
        {
            fullPreviewTexture = LoadTextureFromImage(pendingFullImage);
            UnloadImage(pendingFullImage);
            atomic_store(&isFullTextureReady, true);
            atomic_store(&fullImagePending, false);
        }
        previewAnim = Lerp(previewAnim, isPreviewing ? 1.f : 0.f, delta * g_anim_speed * 0.8f);
        if (isPreviewing) { if (kenBurnsTimer < g_ken_burns_duration) kenBurnsTimer += delta; }

        BeginTextureMode(mainTarget);
        {
            ClearBackground(AppTheme.bg);
            UpdateAndDrawScene(filteredCount, filteredIndices, delta, isPreviewing, isSearching);
            if (isSearching || searchBufferCount > 0)
            {
                const float searchFontSize = 20.0f;
                const float searchSpacing = 1.0f;
                const int pad = 12;

                DrawRectangle(0, 0, sw, (int)SEARCH_UI_HEIGHT, AppTheme.searchBg);
                DrawRectangleLinesEx((Rectangle){0, 0, (float)sw, SEARCH_UI_HEIGHT}, 1.0f, AppTheme.searchBorder);

                char searchPrompt[512];
                if (searchBufferCount > 0)
                {
                    snprintf(searchPrompt, sizeof(searchPrompt), "Search: %s", searchBuffer);
                }
                else
                {
                    snprintf(searchPrompt, sizeof(searchPrompt), "Search: ");
                }

                if (isSearching && fmod(GetTime(), 1.0) > 0.5 && strlen(searchPrompt) < sizeof(searchPrompt) - 1)
                {
                    strcat(searchPrompt, "|");
                }

                Color promptColor = (searchBufferCount > 0 || isSearching) ? AppTheme.searchText : AppTheme.searchHint;
                DrawTextEx(gFont, searchPrompt, (Vector2){(float)pad, 12.0f}, searchFontSize, searchSpacing, promptColor);

                char countLabel[64];
                snprintf(countLabel, sizeof(countLabel), "%d match%s", filteredCount, filteredCount == 1 ? "" : "es");
                Vector2 countSize = MeasureTextEx(gFont, countLabel, 18.0f, 1.0f);
                DrawTextEx(gFont, countLabel, (Vector2){(float)sw - countSize.x - pad, 14.0f}, 18.0f, 1.0f, AppTheme.searchCount);
            }

            if (searchBufferCount > 0 && filteredCount == 0 && !isPreviewing)
            {
                const char *noResults = "No wallpapers match your search";
                const float noResultsSize = 24.0f;
                Vector2 messageSize = MeasureTextEx(gFont, noResults, noResultsSize, 1.0f);
                DrawTextEx(
                    gFont,
                    noResults,
                    (Vector2){(sw - messageSize.x) * 0.5f, (sh - messageSize.y) * 0.5f},
                    noResultsSize,
                    1.0f,
                    AppTheme.searchHint
                );
            }
        }
        EndTextureMode();

        BeginTextureMode(bloomMaskHiRes);
        {
            ClearBackground(BLANK);
            if (g_hoveredIndex != -1 && wallpapers[g_hoveredIndex].hoverAnim > 0.01f)
            {
                Wallpaper w = wallpapers[g_hoveredIndex];
                Rectangle r = {w.animPos.x, w.animPos.y, w.animSize.x, w.animSize.y};
                if (!isPreviewing)
                    DrawRectangleLinesEx(r, g_border_thickness_bloom * 2.f, Fade(AppTheme.border, w.hoverAnim));
            }
        }
        EndTextureMode();

        bool h = true;
        Vector2 rs = {(float)bloomMask.texture.width, (float)bloomMask.texture.height};
        BeginShaderMode(blurShader);
        SetShaderValue(blurShader, GetShaderLocation(blurShader, "renderSize"), &rs, SHADER_UNIFORM_VEC2);
        SetShaderValue(blurShader, GetShaderLocation(blurShader, "horizontal"), &h, SHADER_UNIFORM_INT);
        BeginTextureMode(blurPingPong);
        ClearBackground(BLANK);
        DrawTextureRec(bloomMask.texture, (Rectangle){0, 0, rs.x, -rs.y}, (Vector2){0, 0}, WHITE);
        EndTextureMode();
        h = false;
        SetShaderValue(blurShader, GetShaderLocation(blurShader, "horizontal"), &h, SHADER_UNIFORM_INT);
        BeginTextureMode(bloomMask);
        ClearBackground(BLANK);
        DrawTextureRec(blurPingPong.texture, (Rectangle){0, 0, rs.x, -rs.y}, (Vector2){0, 0}, WHITE);
        EndTextureMode();
        EndShaderMode();

        BeginDrawing();
        {
            ClearBackground(AppTheme.bg);
            BeginShaderMode(postShader);
            float tt = GetTime();
            SetShaderValue(postShader, timeLoc, &tt, SHADER_UNIFORM_FLOAT);
            Vector2 res = {(float)sw, (float)sh};
            SetShaderValue(postShader, resLoc, &res, SHADER_UNIFORM_VEC2);
            float z = 0.f, pv = 0.f; int m = 0; float sl = 0.0f;
            SetShaderValue(postShader, glitchLoc, (g_activeEffect == EFFECT_GLITCH) ? &g_effectIntensity : &z, SHADER_UNIFORM_FLOAT);
            SetShaderValue(postShader, blurLoc, (g_activeEffect == EFFECT_BLUR) ? &g_effectIntensity : &z, SHADER_UNIFORM_FLOAT);
            if (g_activeEffect == EFFECT_PIXELATE) { pv = 256.f * (1.f - g_effectIntensity); if (pv < 10.f) pv = 10.f; }
            SetShaderValue(postShader, pixelLoc, &pv, SHADER_UNIFORM_FLOAT);
            if (g_activeEffect == EFFECT_REVEAL) m = 3;
            SetShaderValue(postShader, mirrorLoc, &m, SHADER_UNIFORM_INT);
            SetShaderValue(postShader, shakeLoc, (g_activeEffect == EFFECT_SHAKE) ? &g_effectIntensity : &z, SHADER_UNIFORM_FLOAT);
            SetShaderValue(postShader, collapseLoc, (g_activeEffect == EFFECT_REVEAL || g_activeEffect == EFFECT_COLLAPSE) ? &g_effectIntensity : &z, SHADER_UNIFORM_FLOAT);
            SetShaderValue(postShader, scanlineLoc, &sl, SHADER_UNIFORM_FLOAT);

            rlActiveTextureSlot(1); rlEnableTexture(bloomMask.texture.id);
            SetShaderValue(postShader, GetShaderLocation(postShader, "bloomTexture"), (int[]){1}, SHADER_UNIFORM_INT);
            rlActiveTextureSlot(2); rlEnableTexture(bloomMaskHiRes.texture.id);
            SetShaderValue(postShader, GetShaderLocation(postShader, "bloomTextureHiRes"), (int[]){2}, SHADER_UNIFORM_INT);

            rlActiveTextureSlot(0);
            DrawTextureRec(mainTarget.texture, (Rectangle){0, 0, (float)sw, (float)-sh}, (Vector2){0, 0}, WHITE);
            EndShaderMode();
        }
        EndDrawing();
    }

    atomic_store(&loader_running, false);
    for (int t = 0; t < g_max_threads; t++)
    {
        pthread_join(loader_threads[t], NULL);
    }
    UnloadShader(postShader);
    UnloadShader(blurShader);
    UnloadRenderTexture(mainTarget);
    UnloadRenderTexture(bloomMask);
    UnloadRenderTexture(blurPingPong);
    UnloadRenderTexture(bloomMaskHiRes);
    for (int i = 0; i < wallpaper_count; i++)
    {
        if (atomic_load(&wallpapers[i].loaded))
        {
            UnloadTexture(wallpapers[i].texture);
        }
        free(wallpapers[i].path);
        free(wallpapers[i].filename);
    }
    CloseWindow();
    return 0;
}
