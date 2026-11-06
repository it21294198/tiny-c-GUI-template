
#ifndef TINYGUI_H
#define TINYGUI_H

/*
 * TinyGUI v0.2
 * Native macOS GUI library
 *
 * Public API: C
 * Backend: Cocoa / AppKit
 *
 * Define TINYGUI_IMPLEMENTATION in ONE source file.
 * Compile using: clang -x objective-c
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================
   PUBLIC TYPES
   ========================================================= */

typedef struct TG_Widget TG_Widget;

typedef enum {
    TG_ALIGN_START,
    TG_ALIGN_CENTER,
    TG_ALIGN_END,
    TG_ALIGN_STRETCH
} TG_Align;

typedef enum {
    TG_VERTICAL,
    TG_HORIZONTAL
} TG_Direction;

typedef struct {
    int width;
    int height;
    const char *title;
} TG_WindowConfig;

/* =========================================================
   WINDOW API
   ========================================================= */

bool tg_window(TG_WindowConfig config);

bool tg_poll(void);

void tg_run(void);

void tg_close(void);

/* =========================================================
   LAYOUT API
   ========================================================= */

void tg_begin(void);
void tg_end(void);

void tg_set_padding(float padding);
void tg_set_spacing(float spacing);
void tg_set_alignment(TG_Align alignment);

void tg_row_begin(void);
void tg_row_end(void);

void tg_column_begin(void);
void tg_column_end(void);

void tg_container_begin(float width, float height);
void tg_container_end(void);

/* =========================================================
   WIDGET API
   ========================================================= */

TG_Widget *tg_label(const char *text);

TG_Widget *tg_button(const char *text);

TG_Widget *tg_textbox(const char *placeholder);

TG_Widget *tg_checkbox(const char *text, bool checked);

TG_Widget *tg_slider(float min, float max, float value);

/* =========================================================
   WIDGET INTERACTION
   ========================================================= */

bool tg_clicked(TG_Widget *widget);

void tg_set_text(TG_Widget *widget, const char *text);

const char *tg_get_text(TG_Widget *widget);

void tg_set_checked(TG_Widget *widget, bool checked);

bool tg_is_checked(TG_Widget *widget);

void tg_set_value(TG_Widget *widget, float value);

float tg_get_value(TG_Widget *widget);

#ifdef __cplusplus
}
#endif

/* =========================================================
   IMPLEMENTATION
   ========================================================= */

#ifdef TINYGUI_IMPLEMENTATION

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* =========================================================
   CONFIGURATION
   ========================================================= */

#define TG_MAX_WIDGETS 256
#define TG_MAX_LAYOUTS 32

#define TG_DEFAULT_PADDING 20.0f
#define TG_DEFAULT_SPACING 12.0f

#define TG_ROW_HEIGHT 44.0f

/* =========================================================
   INTERNAL TYPES
   ========================================================= */

typedef enum {
    TG_TYPE_LABEL,
    TG_TYPE_BUTTON,
    TG_TYPE_TEXTBOX,
    TG_TYPE_CHECKBOX,
    TG_TYPE_SLIDER
} TG_Type;

struct TG_Widget {
    void *native;
    TG_Type type;
    bool clicked;
};

typedef struct {
    float x;
    float y;

    float width;
    float height;

    float cursor;

    float padding;
    float spacing;

    TG_Direction direction;
    TG_Align alignment;

} TG_Layout;

/* =========================================================
   GLOBAL STATE
   ========================================================= */

static NSApplication *tg_app = nil;
static NSWindow *tg_native_window = nil;
static NSView *tg_content = nil;

static NSMutableArray *tg_widgets = nil;
static NSMutableArray *tg_targets = nil;

static TG_Widget *tg_widget_registry[TG_MAX_WIDGETS];

static int tg_widget_count = 0;

static TG_Layout tg_layout_stack[TG_MAX_LAYOUTS];
static int tg_layout_count = 0;

static bool tg_initialized = false;
static bool tg_running = false;

/* =========================================================
   OBJECTIVE-C BUTTON TARGET
   ========================================================= */

@interface TGButtonTarget : NSObject {
    TG_Widget *widget;
}

- (id)initWithWidget:(TG_Widget *)value;

- (void)clicked:(id)sender;

@end

@implementation TGButtonTarget

- (id)initWithWidget:(TG_Widget *)value
{
    self = [super init];

    if (self) {
        widget = value;
    }

    return self;
}

- (void)clicked:(id)sender
{
    (void)sender;

    if (widget) {
        widget->clicked = true;
    }
}

@end

/* =========================================================
   APPLICATION DELEGATE
   ========================================================= */

@interface TGAppDelegate : NSObject
    <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation TGAppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:
    (NSApplication *)application
{
    (void)application;

    return NO;
}

- (void)windowWillClose:(NSNotification *)notification
{
    (void)notification;

    tg_running = false;
}

@end

static TGAppDelegate *tg_delegate = nil;

/* =========================================================
   INTERNAL HELPERS
   ========================================================= */

static NSView *tg_native_view(TG_Widget *widget)
{
    if (!widget || !widget->native)
        return nil;

    return (NSView *)widget->native;
}

static TG_Layout *tg_current_layout(void)
{
    if (tg_layout_count <= 0)
        return NULL;

    return &tg_layout_stack[tg_layout_count - 1];
}

static bool tg_push_layout(TG_Layout layout)
{
    if (tg_layout_count >= TG_MAX_LAYOUTS)
        return false;

    tg_layout_stack[tg_layout_count++] = layout;

    return true;
}

static void tg_pop_layout(void)
{
    if (tg_layout_count > 1)
        tg_layout_count--;
}

/* =========================================================
   WIDGET REGISTRY
   ========================================================= */

static TG_Widget *tg_register_widget(
    NSView *native,
    TG_Type type
)
{
    if (!native || !tg_content)
        return NULL;

    if (tg_widget_count >= TG_MAX_WIDGETS) {
        [native release];
        return NULL;
    }

    TG_Widget *widget = calloc(1, sizeof(TG_Widget));

    if (!widget) {
        [native release];
        return NULL;
    }

    widget->native = native;
    widget->type = type;

    [tg_content addSubview:native];

    /* The content view now retains the native control. */
    [native release];

    tg_widget_registry[tg_widget_count++] = widget;

    [tg_widgets addObject:
        [NSValue valueWithPointer:widget]];

    return widget;
}

/* =========================================================
   LAYOUT ENGINE
   ========================================================= */

static NSRect tg_next_frame(float width, float height)
{
    TG_Layout *layout = tg_current_layout();

    if (!layout || !tg_content)
        return NSZeroRect;

    float x = layout->x + layout->padding;
    float y = layout->y + layout->padding;

    if (layout->direction == TG_HORIZONTAL) {
        x += layout->cursor;

        layout->cursor += width + layout->spacing;

        if (layout->alignment == TG_ALIGN_CENTER)
            y += (layout->height - height) / 2.0f;

        if (layout->alignment == TG_ALIGN_END)
            y += layout->height - height - layout->padding;

        if (layout->alignment == TG_ALIGN_STRETCH)
            height = layout->height - 2.0f * layout->padding;
    }
    else {
        y += layout->cursor;

        layout->cursor += height + layout->spacing;

        if (layout->alignment == TG_ALIGN_CENTER)
            x += (layout->width - width) / 2.0f;

        if (layout->alignment == TG_ALIGN_END)
            x += layout->width - width - layout->padding;

        if (layout->alignment == TG_ALIGN_STRETCH)
            width = layout->width - 2.0f * layout->padding;
    }

    /* Convert top-left layout coordinates to Cocoa coordinates. */

    float content_height =
        (float)tg_content.bounds.size.height;

    float native_y = content_height - y - height;

    return NSMakeRect(x, native_y, width, height);
}

/* =========================================================
   WINDOW IMPLEMENTATION
   ========================================================= */

bool tg_window(TG_WindowConfig config)
{
    if (tg_initialized)
        return false;

    if (config.width <= 0 || config.height <= 0)
        return false;

    if (!config.title)
        return false;

    @autoreleasepool {

        tg_app = [NSApplication sharedApplication];

        [tg_app setActivationPolicy:
            NSApplicationActivationPolicyRegular];

        tg_delegate = [[TGAppDelegate alloc] init];

        [tg_app setDelegate:tg_delegate];

        NSRect frame = NSMakeRect(
            100,
            100,
            config.width,
            config.height
        );

        NSWindowStyleMask style =
            NSWindowStyleMaskTitled |
            NSWindowStyleMaskClosable |
            NSWindowStyleMaskMiniaturizable |
            NSWindowStyleMaskResizable;

        tg_native_window = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:style
            backing:NSBackingStoreBuffered
            defer:NO];

        if (!tg_native_window) {
            [tg_delegate release];
            tg_delegate = nil;

            return false;
        }

        [tg_native_window setTitle:
            [NSString stringWithUTF8String:config.title]];

        [tg_native_window setReleasedWhenClosed:NO];

        [tg_native_window setDelegate:tg_delegate];

        tg_content = [tg_native_window contentView];

        tg_widgets = [[NSMutableArray alloc] init];

        tg_targets = [[NSMutableArray alloc] init];

        tg_widget_count = 0;
        tg_layout_count = 0;

        memset(
            tg_widget_registry,
            0,
            sizeof(tg_widget_registry)
        );

        TG_Layout root = {
            .x = 0,
            .y = 0,
            .width = (float)config.width,
            .height = (float)config.height,
            .cursor = 0,
            .padding = TG_DEFAULT_PADDING,
            .spacing = TG_DEFAULT_SPACING,
            .direction = TG_VERTICAL,
            .alignment = TG_ALIGN_START
        };

        tg_push_layout(root);

        [tg_native_window center];

        [tg_native_window makeKeyAndOrderFront:nil];

        [tg_app activateIgnoringOtherApps:YES];

        tg_initialized = true;
        tg_running = true;
    }

    return true;
}

/* =========================================================
   EVENT LOOP
   ========================================================= */

bool tg_poll(void)
{
    if (!tg_initialized || !tg_running)
        return false;

    @autoreleasepool {

        NSDate *deadline =
            [NSDate dateWithTimeIntervalSinceNow:1.0 / 60.0];

        NSEvent *event = [tg_app
            nextEventMatchingMask:NSEventMaskAny
            untilDate:deadline
            inMode:NSDefaultRunLoopMode
            dequeue:YES];

        if (event) {
            [tg_app sendEvent:event];
        }

        [tg_app updateWindows];

        if (!tg_native_window.isVisible)
            tg_running = false;
    }

    return tg_running;
}

void tg_run(void)
{
    while (tg_poll()) {
        /* Cocoa events are processed in tg_poll(). */
    }
}

/* =========================================================
   LAYOUT API
   ========================================================= */

void tg_begin(void)
{
    if (!tg_initialized)
        return;

    TG_Layout root = {
        .x = 0,
        .y = 0,
        .width = (float)tg_content.bounds.size.width,
        .height = (float)tg_content.bounds.size.height,
        .cursor = 0,
        .padding = TG_DEFAULT_PADDING,
        .spacing = TG_DEFAULT_SPACING,
        .direction = TG_VERTICAL,
        .alignment = TG_ALIGN_START
    };

    tg_layout_count = 0;

    tg_push_layout(root);
}

void tg_end(void)
{
    /* Layout construction is complete. */
}

void tg_set_padding(float padding)
{
    TG_Layout *layout = tg_current_layout();

    if (layout && padding >= 0)
        layout->padding = padding;
}

void tg_set_spacing(float spacing)
{
    TG_Layout *layout = tg_current_layout();

    if (layout && spacing >= 0)
        layout->spacing = spacing;
}

void tg_set_alignment(TG_Align alignment)
{
    TG_Layout *layout = tg_current_layout();

    if (!layout)
        return;

    if (alignment < TG_ALIGN_START ||
        alignment > TG_ALIGN_STRETCH)
        return;

    layout->alignment = alignment;
}

/* =========================================================
   ROW
   ========================================================= */

void tg_row_begin(void)
{
    TG_Layout *parent = tg_current_layout();

    if (!parent)
        return;

    TG_Layout row = {
        .x = parent->x + parent->padding,
        .y = parent->y + parent->padding + parent->cursor,
        .width = parent->width - 2.0f * parent->padding,
        .height = TG_ROW_HEIGHT,
        .cursor = 0,
        .padding = 0,
        .spacing = parent->spacing,
        .direction = TG_HORIZONTAL,
        .alignment = TG_ALIGN_CENTER
    };

    parent->cursor += row.height + parent->spacing;

    tg_push_layout(row);
}

void tg_row_end(void)
{
    tg_pop_layout();
}

/* =========================================================
   COLUMN
   ========================================================= */

void tg_column_begin(void)
{
    TG_Layout *parent = tg_current_layout();

    if (!parent)
        return;

    float available_height =
        parent->height -
        parent->cursor -
        2.0f * parent->padding;

    TG_Layout column = {
        .x = parent->x + parent->padding,
        .y = parent->y + parent->padding + parent->cursor,
        .width = parent->width - 2.0f * parent->padding,
        .height = available_height,
        .cursor = 0,
        .padding = 0,
        .spacing = parent->spacing,
        .direction = TG_VERTICAL,
        .alignment = TG_ALIGN_START
    };

    tg_push_layout(column);
}

void tg_column_end(void)
{
    tg_pop_layout();
}

/* =========================================================
   CONTAINER
   ========================================================= */

void tg_container_begin(float width, float height)
{
    if (width <= 0 || height <= 0)
        return;

    if (tg_layout_count >= TG_MAX_LAYOUTS)
        return;

    NSRect rect = tg_next_frame(width, height);

    float top_y =
        (float)tg_content.bounds.size.height -
        (float)NSMaxY(rect);

    TG_Layout container = {
        .x = (float)rect.origin.x,
        .y = top_y,
        .width = width,
        .height = height,
        .cursor = 0,
        .padding = 12,
        .spacing = 8,
        .direction = TG_VERTICAL,
        .alignment = TG_ALIGN_START
    };

    tg_push_layout(container);
}

void tg_container_end(void)
{
    tg_pop_layout();
}

/* =========================================================
   LABEL
   ========================================================= */

TG_Widget *tg_label(const char *text)
{
    if (!tg_initialized || !text)
        return NULL;

    @autoreleasepool {

        NSTextField *field = [[NSTextField alloc]
            initWithFrame:tg_next_frame(250, 24)];

        [field setStringValue:
            [NSString stringWithUTF8String:text]];

        [field setEditable:NO];
        [field setSelectable:NO];
        [field setBordered:NO];
        [field setDrawsBackground:NO];

        return tg_register_widget(field, TG_TYPE_LABEL);
    }
}

/* =========================================================
   BUTTON
   ========================================================= */

TG_Widget *tg_button(const char *text)
{
    if (!tg_initialized || !text)
        return NULL;

    @autoreleasepool {

        NSButton *button = [[NSButton alloc]
            initWithFrame:tg_next_frame(150, 32)];

        [button setTitle:
            [NSString stringWithUTF8String:text]];

        [button setButtonType:NSButtonTypeMomentaryPushIn];

        [button setBezelStyle:NSBezelStyleRounded];

        TG_Widget *widget =
            tg_register_widget(button, TG_TYPE_BUTTON);

        if (!widget)
            return NULL;

        TGButtonTarget *target =
            [[TGButtonTarget alloc] initWithWidget:widget];

        [tg_targets addObject:target];

        [button setTarget:target];

        [button setAction:@selector(clicked:)];

        [target release];

        return widget;
    }
}

/* =========================================================
   TEXTBOX
   ========================================================= */

TG_Widget *tg_textbox(const char *placeholder)
{
    if (!tg_initialized)
        return NULL;

    @autoreleasepool {

        NSTextField *field = [[NSTextField alloc]
            initWithFrame:tg_next_frame(250, 30)];

        if (placeholder) {
            [field setPlaceholderString:
                [NSString stringWithUTF8String:placeholder]];
        }

        return tg_register_widget(field, TG_TYPE_TEXTBOX);
    }
}

/* =========================================================
   CHECKBOX
   ========================================================= */

TG_Widget *tg_checkbox(const char *text, bool checked)
{
    if (!tg_initialized || !text)
        return NULL;

    @autoreleasepool {

        NSButton *button = [[NSButton alloc]
            initWithFrame:tg_next_frame(200, 24)];

        [button setTitle:
            [NSString stringWithUTF8String:text]];

        [button setButtonType:NSButtonTypeSwitch];

        [button setState:
            checked ? NSControlStateValueOn :
                      NSControlStateValueOff];

        return tg_register_widget(button, TG_TYPE_CHECKBOX);
    }
}

/* =========================================================
   SLIDER
   ========================================================= */

TG_Widget *tg_slider(float min, float max, float value)
{
    if (!tg_initialized || max <= min)
        return NULL;

    @autoreleasepool {

        NSSlider *slider = [[NSSlider alloc]
            initWithFrame:tg_next_frame(250, 28)];

        [slider setMinValue:min];
        [slider setMaxValue:max];
        [slider setFloatValue:value];

        return tg_register_widget(slider, TG_TYPE_SLIDER);
    }
}

/* =========================================================
   BUTTON INTERACTION
   ========================================================= */

bool tg_clicked(TG_Widget *widget)
{
    if (!widget || widget->type != TG_TYPE_BUTTON)
        return false;

    bool clicked = widget->clicked;

    widget->clicked = false;

    return clicked;
}

/* =========================================================
   TEXT API
   ========================================================= */

void tg_set_text(TG_Widget *widget, const char *text)
{
    if (!widget || !text)
        return;

    @autoreleasepool {

        if (widget->type != TG_TYPE_TEXTBOX &&
            widget->type != TG_TYPE_LABEL)
            return;

        NSTextField *field =
            (NSTextField *)tg_native_view(widget);

        [field setStringValue:
            [NSString stringWithUTF8String:text]];
    }
}

const char *tg_get_text(TG_Widget *widget)
{
    if (!widget)
        return "";

    if (widget->type != TG_TYPE_TEXTBOX &&
        widget->type != TG_TYPE_LABEL)
        return "";

    NSTextField *field =
        (NSTextField *)tg_native_view(widget);

    return [[field stringValue] UTF8String];
}

/* =========================================================
   CHECKBOX API
   ========================================================= */

void tg_set_checked(TG_Widget *widget, bool checked)
{
    if (!widget || widget->type != TG_TYPE_CHECKBOX)
        return;

    NSButton *button =
        (NSButton *)tg_native_view(widget);

    [button setState:
        checked ? NSControlStateValueOn :
                  NSControlStateValueOff];
}

bool tg_is_checked(TG_Widget *widget)
{
    if (!widget || widget->type != TG_TYPE_CHECKBOX)
        return false;

    NSButton *button =
        (NSButton *)tg_native_view(widget);

    return [button state] == NSControlStateValueOn;
}

/* =========================================================
   SLIDER API
   ========================================================= */

void tg_set_value(TG_Widget *widget, float value)
{
    if (!widget || widget->type != TG_TYPE_SLIDER)
        return;

    NSSlider *slider =
        (NSSlider *)tg_native_view(widget);

    [slider setFloatValue:value];
}

float tg_get_value(TG_Widget *widget)
{
    if (!widget || widget->type != TG_TYPE_SLIDER)
        return 0.0f;

    NSSlider *slider =
        (NSSlider *)tg_native_view(widget);

    return [slider floatValue];
}

/* =========================================================
   CLEANUP
   ========================================================= */

void tg_close(void)
{
    if (!tg_initialized)
        return;

    @autoreleasepool {

        tg_running = false;

        if (tg_native_window) {
            [tg_native_window setDelegate:nil];
            [tg_native_window close];
        }

        if (tg_app) {
            [tg_app setDelegate:nil];
        }

        for (int i = 0; i < tg_widget_count; i++) {
            free(tg_widget_registry[i]);
            tg_widget_registry[i] = NULL;
        }

        tg_widget_count = 0;
        tg_layout_count = 0;

        [tg_widgets release];
        [tg_targets release];

        [tg_native_window release];
        [tg_delegate release];

        tg_widgets = nil;
        tg_targets = nil;

        tg_native_window = nil;
        tg_content = nil;
        tg_delegate = nil;
        tg_app = nil;

        tg_initialized = false;
    }
}

#endif /* TINYGUI_IMPLEMENTATION */

#endif /* TINYGUI_H */

