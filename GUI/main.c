
#define TINYGUI_IMPLEMENTATION
#include "tinygui.h"

#include <stdio.h>

int main(void)
{
    TG_WindowConfig config = {
        .width = 900,
        .height = 650,
        .title = "TinyGUI - Native macOS"
    };

    if (!tg_window(config)) {
        printf("Failed to initialize TinyGUI\n");
        return 1;
    }

    // Build the UI once.
    tg_begin();

    tg_set_padding(25);
    tg_set_spacing(15);

    tg_label("TinyGUI Dashboard");

    tg_label("Native macOS controls using Cocoa");

    // Horizontal row.
    tg_row_begin();

    TG_Widget *increment = tg_button("Increment");
    TG_Widget *reset = tg_button("Reset");

    tg_row_end();

    // Text input.
    TG_Widget *name = tg_textbox("Enter your name");

    // Checkbox.
    TG_Widget *notifications =
        tg_checkbox("Enable notifications", true);

    // Slider.
    tg_label("Volume");

    TG_Widget *volume = tg_slider(0, 100, 50);

    tg_end();

    int counter = 0;

    // Process events and application logic.
    while (tg_poll()) {

        if (tg_clicked(increment)) {
            counter++;
            printf("Counter: %d\n", counter);
        }

        if (tg_clicked(reset)) {
            counter = 0;
            printf("Counter reset\n");
        }

        // Read current widget values.
        (void)tg_get_text(name);
        (void)tg_is_checked(notifications);
        (void)tg_get_value(volume);
    }

    printf("\nFinal values:\n");
    printf("Name: %s\n", tg_get_text(name));

    printf("Notifications: %s\n",
        tg_is_checked(notifications) ? "ON" : "OFF");

    printf("Volume: %.2f\n", tg_get_value(volume));

    tg_close();

    return 0;
}

