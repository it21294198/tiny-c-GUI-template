
#ifndef TINYGUI_H
#define TINYGUI_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int width;
    int height;
    const char *title;
} TG_WindowConfig;

bool tg_init(TG_WindowConfig config);
bool tg_begin_frame(void);
void tg_end_frame(void);
void tg_shutdown(void);

#ifdef __cplusplus
}
#endif

/* =========================================================
   IMPLEMENTATION
   Define TINYGUI_IMPLEMENTATION in ONE C file.
   ========================================================= */

#ifdef TINYGUI_IMPLEMENTATION

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>

#include <stdint.h>
#include <stdio.h>

/* Minimal Cocoa-compatible geometry structures */

typedef struct {
    double x;
    double y;
} TG_Point;

typedef struct {
    double width;
    double height;
} TG_Size;

typedef struct {
    TG_Point origin;
    TG_Size size;
} TG_Rect;

/* Cocoa constants */

#define TG_WINDOW_TITLED        (1UL << 0)
#define TG_WINDOW_CLOSABLE      (1UL << 1)
#define TG_WINDOW_MINIATURIZABLE (1UL << 2)
#define TG_WINDOW_RESIZABLE     (1UL << 3)

#define TG_BACKING_BUFFERED 2
#define TG_ACTIVATION_REGULAR 0

/* Objective-C runtime function signatures */

#define TG_MSG(return_type, name, ...) \
    ((return_type (*)(id, SEL, ##__VA_ARGS__))objc_msgSend)

#define TG_CLASS(name) objc_getClass(name)
#define TG_SEL(name) sel_registerName(name)

static id tg_app = nil;
static id tg_window = nil;
static bool tg_initialized = false;

/* Create NSString from C string */

static id tg_string(const char *text)
{
    return TG_MSG(id, "stringWithUTF8String:", const char *)
        ((id)TG_CLASS("NSString"),
         TG_SEL("stringWithUTF8String:"),
         text);
}

/* Initialize application and create native window */

bool tg_init(TG_WindowConfig config)
{
    if (tg_initialized)
        return false;

    if (config.width <= 0 || config.height <= 0)
        return false;

    if (!config.title)
        return false;

    /* Get NSApplication */

    id app_class = (id)TG_CLASS("NSApplication");

    tg_app = TG_MSG(id, "sharedApplication")
        (app_class, TG_SEL("sharedApplication"));

    if (!tg_app)
        return false;

    /* Activate application */

    TG_MSG(void, "setActivationPolicy:", long)
        (tg_app, TG_SEL("setActivationPolicy:"),
         TG_ACTIVATION_REGULAR);

    /* Create window rectangle */

    TG_Rect rect = {
        .origin = { 100, 100 },
        .size = {
            (double)config.width,
            (double)config.height
        }
    };

    unsigned long style =
        TG_WINDOW_TITLED |
        TG_WINDOW_CLOSABLE |
        TG_WINDOW_MINIATURIZABLE |
        TG_WINDOW_RESIZABLE;

    /* Allocate NSWindow */

    id window_class = (id)TG_CLASS("NSWindow");

    tg_window = TG_MSG(id, "alloc")
        (window_class, TG_SEL("alloc"));

    tg_window = TG_MSG(id, "initWithContentRect:styleMask:backing:defer:",
                       TG_Rect, unsigned long, unsigned long, bool)
        (tg_window,
         TG_SEL("initWithContentRect:styleMask:backing:defer:"),
         rect,
         style,
         TG_BACKING_BUFFERED,
         false);

    if (!tg_window)
        return false;

    /* Set window title */

    id title = tg_string(config.title);

    TG_MSG(void, "setTitle:", id)
        (tg_window, TG_SEL("setTitle:"), title);

    /* Keep window alive after close */

    TG_MSG(void, "setReleasedWhenClosed:", bool)
        (tg_window, TG_SEL("setReleasedWhenClosed:"), false);

    /* Show window */

    TG_MSG(void, "makeKeyAndOrderFront:", id)
        (tg_window, TG_SEL("makeKeyAndOrderFront:"), nil);

    /* Activate application */

    TG_MSG(void, "activateIgnoringOtherApps:", bool)
        (tg_app, TG_SEL("activateIgnoringOtherApps:"), true);

    tg_initialized = true;

    return true;
}

/* Process macOS events */

bool tg_begin_frame(void)
{
    if (!tg_initialized || !tg_window)
        return false;

    id date_class = (id)TG_CLASS("NSDate");

    id date = TG_MSG(id, "dateWithTimeIntervalSinceNow:", double)
        (date_class,
         TG_SEL("dateWithTimeIntervalSinceNow:"),
         1.0 / 60.0);

    /* Default Cocoa run loop mode */

    extern id NSDefaultRunLoopMode;

    id event = TG_MSG(id, "nextEventMatchingMask:untilDate:inMode:dequeue:",
                      unsigned long, id, id, bool)
        (tg_app,
         TG_SEL("nextEventMatchingMask:untilDate:inMode:dequeue:"),
         ~0UL,
         date,
         NSDefaultRunLoopMode,
         true);

    if (event) {
        TG_MSG(void, "sendEvent:", id)
            (tg_app, TG_SEL("sendEvent:"), event);
    }

    TG_MSG(void, "updateWindows")
        (tg_app, TG_SEL("updateWindows"));

    bool visible = TG_MSG(bool, "isVisible")
        (tg_window, TG_SEL("isVisible"));

    return visible;
}

/* End frame */

void tg_end_frame(void)
{
    if (!tg_initialized)
        return;

    TG_MSG(void, "updateWindows")
        (tg_app, TG_SEL("updateWindows"));
}

/* Cleanup */

void tg_shutdown(void)
{
    if (!tg_initialized)
        return;

    if (tg_window) {
        TG_MSG(void, "close")
            (tg_window, TG_SEL("close"));

        tg_window = nil;
    }

    tg_app = nil;
    tg_initialized = false;
}

#endif /* TINYGUI_IMPLEMENTATION */

#endif /* TINYGUI_H */

