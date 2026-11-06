
#define TINYGUI_IMPLEMENTATION
#include "tinygui.h"

int main(void)
{
    TG_WindowConfig config = {
        .width = 900,
        .height = 600,
        .title = "TinyGUI - Native macOS"
    };

    if (!tg_init(config)) {
        printf("Failed to initialize TinyGUI\n");
        return 1;
    }

    printf("TinyGUI initialized successfully!\n");

    while (tg_begin_frame()) {
        // Application update logic goes here.

        tg_end_frame();
    }

    tg_shutdown();

    return 0;
}

