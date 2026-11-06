### This version includes:
- Native macOS window.
- Labels and buttons.
- Editable text fields.
- Checkboxes.
- Sliders.
- Button click detection.
- Text and widget state access.
- Nested rows, columns and containers.
- Padding, spacing and alignment.
- Manual Cocoa event polling.
Important: This is a C API implemented using Objective-C and AppKit. Compile it using Clang's Objective-C mode. You don't need any third-party libraries.

### Compile
From your GUI directory:
```bash
clang -x objective-c -std=c11 \
    -fno-objc-arc \
    -Wall -Wextra \
    main.c \
    -o tinygui \
    -framework AppKit \
    -framework Foundation
```

Run:
```bash
./tinygui
```

4. Project structure
```bash
tinyGUI/
│
├── CLI/
│   ├── main.c
│   ├── tinygui.h
│   └── README.md
│
└── GUI/
    ├── main.c
    ├── tinygui.h
    └── README.md
```

5. Important implementation notes
```bash
Feature	Current implementation
Native window	Implemented
Native labels	Implemented
Native buttons	Implemented
Button callbacks/state	Implemented
Text fields	Implemented
Checkboxes	Implemented
Sliders	Implemented
Rows and columns	Basic
Nested containers	Basic
Padding and spacing	Implemented
Responsive resizing	Not implemented
```

Two usage details:

1. Call tg_begin(), create your widgets, and call tg_end() once. This version retains native widgets rather than recreating them every frame.
2. Use tg_poll() when you need to inspect button clicks or widget values. tg_run() is a blocking event loop and does not give your application a chance to execute its own per-event logic.

* This is our initial consolidated implementation, not yet a fully tested production release. In particular, layout resizing and advanced nested layout behavior still need further testing.
