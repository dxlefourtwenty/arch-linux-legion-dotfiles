#include "hellpaper.h"
#include <math.h>
#include <strings.h>

void TriggerParticleBurst(Vector2 pos)
{
    for (int i = 0; i < g_particle_count; i++)
    {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(200, 400);
        particles[i] = (Particle){
            .pos = pos,
            .vel = {sinf(angle) * speed, cosf(angle) * speed},
            .life = 1.0f,
            .color = Fade(AppTheme.ripple, 0.5f + (float)GetRandomValue(0, 50) / 100.0f)
        };
    }
}

void TriggerEffect(EffectType type, float duration)
{
    if (type == EFFECT_NONE)
    {
        return;
    }
    g_activeEffect = type;
    g_effectTimer = duration;
    g_effectDuration = duration;
}

EffectType ParseEffect(const char* arg)
{
    if (strcasecmp(arg, "none") == 0) return EFFECT_NONE;
    if (strcasecmp(arg, "glitch") == 0) return EFFECT_GLITCH;
    if (strcasecmp(arg, "blur") == 0) return EFFECT_BLUR;
    if (strcasecmp(arg, "pixelate") == 0) return EFFECT_PIXELATE;
    if (strcasecmp(arg, "reveal") == 0) return EFFECT_REVEAL;
    if (strcasecmp(arg, "shake") == 0) return EFFECT_SHAKE;
    if (strcasecmp(arg, "collapse") == 0) return EFFECT_COLLAPSE;

    LogMessage(LOG_WARNING, "Unrecognized effect '%s'. Defaulting to 'none'.", arg);
    return EFFECT_NONE;
}
