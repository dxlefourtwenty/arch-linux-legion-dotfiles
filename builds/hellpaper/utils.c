#include "hellpaper.h"
#include <ctype.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void LogMessage(int level, const char *format, ...)
{
    const char* level_str = "";
    const char* color_code = "";
    const char* color_reset = "\033[0m";

    switch (level) {
        case LOG_INFO:
            level_str = "INFO";
            color_code = "\033[0;34m";
            break;
        case LOG_WARNING:
            level_str = "WARN";
            color_code = "\033[0;33m";
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            color_code = "\033[0;31m";
            break;
    }

    fprintf(stderr, "%s[%s] ", color_code, level_str);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "%s\n", color_reset);
}

const char *get_home_dir(void)
{
    const char *home = getenv("HOME");
    if (!home)
    {
        struct passwd *pw = getpwuid(getuid());
        if (pw)
        {
            home = pw->pw_dir;
        }
    }
    return home ? home : ".";
}

const char* stristr(const char* haystack, const char* needle)
{
    if (!needle || !*needle)
    {
        return haystack;
    }
    for (; *haystack; ++haystack)
    {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && (tolower((unsigned char)*h) == tolower((unsigned char)*n)))
        {
            h++;
            n++;
        }
        if (!*n)
        {
            return haystack;
        }
    }
    return NULL;
}

char* trim_whitespace(char* str)
{
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void FormatDisplayName(const char *filename, char *out, size_t outSize)
{
    if (!filename || !out) return;

    char temp[512];
    strncpy(temp, filename, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';

    char *dot = strrchr(temp, '.');
    if (dot) *dot = '\0';

    char *start = temp;

    while (*start && isdigit((unsigned char)*start))
        start++;

    if (*start == '-')
        start++;

    for (char *p = start; *p; p++)
    {
        if (*p == '-')
            *p = ' ';
    }

    strncpy(out, start, outSize);
    out[outSize - 1] = '\0';
}
