// Compile with g++ -Wall multi_window_test.cpp XCB_framebuffer_window.cpp -o multi_window_test.exec -lxcb -lxcb-image -lxcb-shm -lxcb-icccm
#include <cstddef>
#include <cstring>
#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "XCB_framebuffer_window.h"

struct flags
{
    bool lock = false, close = false, update = false, loop = true;
} global_flags;

struct window_data
{
    unsigned int window_count;
    class Framebuffer_window ** window_class_ptr_ptr;
};

void * window_thread(void * args)
{
    struct window_data * windows = (struct window_data *)args;
    while (true)
    {
        if (global_flags.close) return NULL;
        for (int i = 0; i < windows->window_count; i ++)
        {
            if (windows->window_class_ptr_ptr[i]->handle_events() < 0) 
            {
                global_flags.close = true;
                return NULL;
            }
        }
    }
}

int main(int argc, char * argv[])
{
    // TODO - read projector resolution from config file
    #define PROJECTOR_WIDTH     1024
    #define PROJECTOR_HEIGHT    768

    struct window_props display_window_props;
    class Framebuffer_window display_window(PROJECTOR_WIDTH, PROJECTOR_HEIGHT, "Projector display.", 18, &display_window_props);
    if (display_window_props.error_status < 0)
    {
        std::cerr << "Failed to create projector display window.\n";
        return -1;
    }

    struct window_props preview_window_props;
    class Framebuffer_window preview_window(PROJECTOR_WIDTH, PROJECTOR_HEIGHT, "Preview.", 8, &preview_window_props);
    if (preview_window_props.error_status < 0)
    {
        std::cerr << "Failed to create preview window.\n";
        return -1;
    }

    preview_window.hide();

    class Framebuffer_window * window_class_ptr_array[] = {&display_window, &preview_window};
    struct window_data window_data = {2, window_class_ptr_array};
    pthread_t window_thread_id;
    if (pthread_create(&window_thread_id, NULL, window_thread, &window_data) != 0)
    {
        std::cerr << "Error: Failed to create window handling thread.";
        return -1;
    }

    char * command_buffer;
    while (!global_flags.close)
    {
        if ((command_buffer = readline(">> ")) == NULL)
        {
            std::cerr << "Error: Received null pointer buffer.\n";
            return -1;
        }
        if (strlen(command_buffer) > 0)
        {
            add_history(command_buffer);
        }
        if (strcmp(command_buffer, "quit") == 0)
        {
            global_flags.close = true;
        }
        free(command_buffer);
    }

    pthread_join(window_thread_id, NULL);

    return 0;
}