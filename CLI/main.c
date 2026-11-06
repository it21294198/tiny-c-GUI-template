
#define TINYGUI_IMPLEMENTATION
#include "tinygui.h"

int main(void)
{
    tg_init();

    int counter = 0;
    bool dark_mode = false;
    bool notifications = true;

    while (tg_running())
    {
        tg_begin_frame();

        // Header
        tg_text(25, 2, "TINYGUI - C TERMINAL GUI");

        tg_text(25, 3, "==========================");

        // Main panel
        tg_panel(15, 5, 50, 14, "Dashboard");

        tg_text(19, 7, "Welcome to TinyGUI!");

        // Counter
        char counter_text[40];
        snprintf(counter_text, sizeof(counter_text),
                 "Counter: %d", counter);

        tg_text(19, 9, counter_text);

        // Buttons
        if (tg_button(19, 11, 20, "Increment"))
            counter++;

        if (tg_button(19, 13, 20, "Reset"))
            counter = 0;

        // Checkboxes
        tg_checkbox(19, 15, "Dark mode", &dark_mode);

        tg_checkbox(19, 16, "Notifications",
                    &notifications);

        // Footer
        tg_text(15, 21, "UP/DOWN: Navigate | ENTER: Select | Q: Quit");

        tg_end_frame();
    }

    tg_shutdown();

    return 0;
}

