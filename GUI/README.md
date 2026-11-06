### Compile on your Mac
No Homebrew installation is required.
```bash
clang -std=c11 -Wall -Wextra main.c \
    -o tinygui \
    -framework AppKit \
    -framework Foundation \
    -lobjc
```

### Run:
```bash
./tinygui
```


You should get a native macOS window measuring 900 × 600 pixels.
### Important: Add an autorelease pool
Because we're calling Cocoa from C, we should manage Objective-C autoreleased objects.

Add this global variable:
```bash
static id tg_pool = nil;
```

At the beginning of tg_init(), before creating NSApplication:
```bash
id pool_class = (id)TG_CLASS("NSAutoreleasePool");

tg_pool = TG_MSG(id, "alloc")
    (pool_class, TG_SEL("alloc"));

tg_pool = TG_MSG(id, "init")
    (tg_pool, TG_SEL("init"));


At the end of tg_shutdown(), before returning:
if (tg_pool) {
    TG_MSG(void, "drain")
        (tg_pool, TG_SEL("drain"));

    tg_pool = nil;
}
```

### Development roadmap
1. Current
    Native window management
   
   Window creation, event loop and cleanup.
2. Native widgets
   
   Labels, buttons, text fields, checkboxes and sliders using Cocoa controls.
3. TinyGUI rendering API
   
   tg_button(), tg_label(), tg_checkbox() and tg_textbox().
4. Layout engine
   
   Rows, columns, padding, alignment and containers.
5. Styling engine
   
   Colors, typography, themes and custom styles.
Our design goal: Application developers should only need tinygui.h and a C compiler. TinyGUI itself will handle the native macOS integration.

One distinction: the implementation is a single-header C API, but it links to Apple's system frameworks and Objective-C runtime. It is not a standard-C-only implementation.
