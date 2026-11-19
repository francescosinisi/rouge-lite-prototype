#ifndef HOTRELOAD_H
#define HOTRELOAD_H

#include "game.h"
#include <time.h>

typedef void (*GameFn)(GameState *);

typedef struct {
    GameFn  init;
    GameFn  update;
    GameFn  shutdown;
    void   *lib;
    char    libPath[64];
    time_t  buildTime;
    int     loaded;
} HotReload;

// Call once per frame. Swaps in game.so if a new build is ready, preserving
// all game state; runs game_init on the first successful load only.
// Returns 1 when hl->update(state) may be called.
int hot_reload_check(HotReload *hl, GameState *state);

// Shuts the game down, unloads the library, and cleans up.
void hot_reload_stop(HotReload *hl, GameState *state);

#endif /* HOT_RELOAD_H */

#ifdef HOT_RELOAD_IMPLEMENTATION
#ifndef HOT_RELOAD_IMPLEMENTATION_ONCE
#define HOT_RELOAD_IMPLEMENTATION_ONCE

#include <dlfcn.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#define BUILD_PATH "build/game.so"

static int copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb");
    if (!in) return 0;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return 0; }

    char buf[8192];
    size_t n;
    int ok = 1;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        if (fwrite(buf, 1, n, out) != n) { ok = 0; break; }
    if (ferror(in)) ok = 0;

    fclose(in);
    if (fclose(out) != 0) ok = 0;
    return ok;
}

int hot_reload_check(HotReload *hl, GameState *state)
{
    struct stat st;
    if (stat(BUILD_PATH, &st) != 0 || st.st_mtime == hl->buildTime)
        return hl->loaded;

    // Stage under a fresh name each time: dlopen would otherwise return the
    // cached handle of the still-loaded previous build and skip the reload.
    // The staged file of the old build is deleted only after it is unloaded,
    // since overwriting a still-mapped library file is fatal.
    static int counter;
    char nextPath[64];
    snprintf(nextPath, sizeof nextPath, "build/game.live%d.so", counter++);
    int staged = copy_file(BUILD_PATH, nextPath);
    void *newLib = staged ? dlopen(nextPath, RTLD_NOW) : NULL;

    if (!newLib)
    {
        fprintf(stderr, "reload failed: %s\n",
                staged ? dlerror() : "could not stage new build");
        if (staged) remove(nextPath);
        hl->buildTime = st.st_mtime;
        return hl->loaded;
    }

    GameFn init     = (GameFn)dlsym(newLib, "game_init");
    GameFn update   = (GameFn)dlsym(newLib, "game_update");
    GameFn shutdown = (GameFn)dlsym(newLib, "game_shutdown");
    if (!init || !update || !shutdown)
    {
        fprintf(stderr, "reload failed: missing game functions\n");
        dlclose(newLib);
        remove(nextPath);
        hl->buildTime = st.st_mtime;
        return hl->loaded;
    }

    if (hl->lib)
    {
        dlclose(hl->lib);
        remove(hl->libPath);
    }
    hl->lib      = newLib;
    hl->init     = init;
    hl->update   = update;
    hl->shutdown = shutdown;
    snprintf(hl->libPath, sizeof hl->libPath, "%s", nextPath);
    hl->buildTime = st.st_mtime;
    printf("%s\n", hl->loaded ? "game reloaded" : "game loaded");
    if (!hl->loaded)
        hl->init(state);    // first load only; reloads reuse loaded assets
    hl->loaded = 1;
    return 1;
}

void hot_reload_stop(HotReload *hl, GameState *state)
{
    if (!hl->lib) return;
    if (hl->loaded)
        hl->shutdown(state);
    dlclose(hl->lib);
    remove(hl->libPath);
    *hl = (HotReload){0};
}

#endif /* HOT_RELOAD_IMPLEMENTATION_ONCE */
#endif /* HOT_RELOAD_IMPLEMENTATION */
