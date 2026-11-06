### Compile
From your tinygui directory:
```bash
clang -std=c99 -Wall -Wextra main.c -o tinygui
```

No linker flags or external libraries are needed.

Run:
```bash
./tinygui
```


### Expected interface
```bash
Your terminal will look approximately like this:
             TINYGUI - C TERMINAL GUI
             ==========================

               +Dashboard--------------------------+
               |                                   |
               | Welcome to TinyGUI!               |
               |                                   |
               | Counter: 0                        |
               |                                   |
               | >[Increment]                      |
               |                                   |
               |  [Reset]                          |
               |                                   |
               |  [ ] Dark mode                    |
               |  [X] Notifications                 |
               +-----------------------------------+

               UP/DOWN: Navigate | ENTER: Select | Q: Quit
```
        
### Current API
```bash
Function	Purpose
tg_init()	Initialize terminal
tg_shutdown()	Restore terminal
tg_begin_frame()	Start frame
tg_end_frame()	Render frame
tg_text()	Draw text
tg_panel()	Draw bordered panel
tg_button()	Interactive button
tg_checkbox()	Interactive checkbox
tg_running()	Check application state
```

Note: This first version uses a fixed 80 × 24 character drawing buffer and Unix terminal APIs available on macOS.
### Future development roadmap
I'd develop TinyGUI in these stages:
1. Stage 1 – Core
   Implemented
   Terminal initialization, rendering, buttons, checkboxes and keyboard navigation.
2. Stage 2 – Layout system
   
   Automatic positioning, rows, columns, padding, spacing and alignment.
3. Stage 3 – Widgets
   
   Text input, dropdown, slider, progress bar, radio buttons and tables.
4. Stage 4 – Styling
   
   Colors, borders, themes and reusable styles.
5. Stage 5 – Advanced rendering
   
   Double buffering, terminal resizing, mouse support and Unicode.
