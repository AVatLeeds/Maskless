// Compile with g++ -Wall main.cpp XCB_framebuffer_window.cpp -o main.exec -lxcb -lxcb-image -lxcb-shm -lxcb-icccm -lreadline
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "XCB_framebuffer_window.h"
#include "PNG_reader.h"

#define PROJECTOR_WIDTH     1024
#define PROJECTOR_HEIGHT    768

#define PIXEL_PER_MM    20

enum blending_modes {OVERWRITE, ADD, SUBTRACT, MULTIPLY};
char * blend_mode_strings[] = {"OVERWRITE", "ADD", "SUBTRACT", "MULTIPLY"};

struct coordinate
{
    unsigned int x, y;
};

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

//uint8_t * generate_grayscale_cal_grid(unsigned int width, unsigned int height, unsigned int pixel_per_mm, uint8_t brightness)
//{
//    unsigned int mm_wide = width / pixel_per_mm;
//    unsigned int cm_wide = width / (pixel_per_mm * 10);
//    unsigned int mm_high = height / pixel_per_mm;
//    unsigned int cm_high = height / (pixel_per_mm * 10);
//    uint8_t * grid_buffer_ptr = (uint8_t *)malloc(width * height * sizeof(uint8_t));
//}

//void write_to_window_framebuffer(uint8_t * framebuffer_ptr, const unsigned int (&channels)[3])
//{
//
//}

//void draw_rectangle(struct window_props window_props, uint8_t * framebuffer_ptr, struct coordinate corner_1, struct coordinate corner_2, unsigned int line_width_px, uint8_t (&channels)[3])
//{
//
//}
struct window_drawing
{
    class Framebuffer_window window;
    void clear_window_framebuffer();
    void fill_window_framebuffer(uint8_t (&channels)[3], enum blending_modes blend_mode);
    void put_pixel(struct coordinate position, uint8_t (&channels)[3], enum blending_modes blend_mode);
    void line_horiz(struct coordinate start_position, int length, unsigned int line_width_px, uint8_t (&channels)[3], enum blending_modes blend_mode);
    void line_vert(struct coordinate start_position, int length, unsigned int line_width_px, uint8_t (&channels)[3], enum blending_modes blend_mode);
};

/*enum commands {QUIT, PREVIEW, CLEAR, FILL, PUT_PIXEL, LINE_HORIZ, LINE_VERT};

struct generic_args
{
    class Framebuffer_window * window_ptr;
    struct window_props * window_properties;
    bool bool_state;
    struct coordinate pos_1, pos_2;
    unsigned int dim_1, dim_2, dim_3, dim_4;
    uint8_t channels[3];
    enum blending_modes blending_mode;
};

struct command
{
    enum commands command_id;
    unsigned int name_length;
    char * name_string;
    unsigned int num_args;
    void (* function)(struct generic_args * args);
};


void quit_command(struct generic_args * args)
{
    global_flags.close = true;
}

void preview_command(struct generic_args * args)
{
    if (args->bool_state)
    {
        args->window_ptr->show();
    }
    else
    {
        args->window_ptr->hide();
    }
}*/

enum arg_types {PATH, TEXT_BOOL, UINT, INT, CSV_UINT_LIST, CSV_INT_LIST, ENUM};

struct generic_args
{
    struct window_data * window_data;
    class PNG * image;
};

struct text_bool
{
    char * option_true;
    char * option_false;
};

struct arg
{
    enum arg_types arg_type;
    union
    {
        char * path;
        struct text_bool text_bool;
        unsigned int list_length;
        char ** enumeration_strings;
    };
};

struct exposed_func
{
    char * name_string;
    unsigned int name_length;
    unsigned int num_args;
    struct generic_args generic_args;
    const struct arg (&args_list)[];
    int (* func)(struct generic_args * generic_args_ptr, void * args_list[]);
};

int quit_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    global_flags.close = true;
    return 0;
}

/*int preview_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    if ((bool *)args_list[0])
    {
        generic_args_ptr->window_ptr->show();
    }
    else
    {
        generic_args_ptr->window_ptr->hide();
    }
    return 0;
}*/

int png_load_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    return (generic_args_ptr->image->open((char *)args_list[0]));
}

/*int handle_args(char * command_buffer, struct exposed_func * func)
{
    void * args_list[func->num_args];
    char * temp_string_ptr;
    void * temp_data_ptr;
    unsigned int arg_idx, i = 0;
    for (arg_idx = 0; arg_idx < func->num_args; arg_idx ++)
    {
        // absorb whitespace
        while ((*command_buffer == ' ') || (*command_buffer == '\t')) command_buffer ++;
        switch (func->args_list[arg_idx].arg_type)
        {
            case TEXT_BOOL:
            temp_data_ptr = malloc(sizeof(bool));
            temp_string_ptr = func->args_list[arg_idx].text_bool.option_true;
            while ((temp_string_ptr[i] != '\0') && (temp_string_ptr[i] == command_buffer[i])) i ++;
            if (temp_string_ptr[i] == '\0')
            {
                *((bool *)temp_data_ptr) = true;
                args_list[arg_idx] = temp_data_ptr;
            }
            else
            {
                i = 0;
                temp_string_ptr = func->args_list[arg_idx].text_bool.option_false;
                while ((temp_string_ptr[i] != '\0') && (temp_string_ptr[i] == command_buffer[i])) i ++;
                if (temp_string_ptr[i] == '\0')
                {
                    *((bool *)temp_data_ptr) = false;
                    args_list[arg_idx] = temp_data_ptr;
                }
                else
                {
                    std::cerr << "Error: Failed to interpret boolean text argument.\n";
                    std::cerr << "\tExpected either \"" << func->args_list[arg_idx].text_bool.option_true << "\" or \"" << func->args_list[arg_idx].text_bool.option_false << std::endl;
                    goto FAIL;
                }
            }

            break;
            case UINT:

            break;

            case INT:
            break;

            case CSV_UINT_LIST:
            break;

            case CSV_INT_LIST:
            break;

            case ENUM:
            break;
        }
    }

    func->func(&(func->generic_args), args_list);
    for (arg_idx = 0; arg_idx < func->num_args; arg_idx ++) free(args_list[arg_idx]);
    return 0;

    FAIL:
    for (arg_idx = 0; arg_idx < func->num_args; arg_idx ++) free(args_list[arg_idx]);
    return -1;
}*/

int main(int argc, char * argv[])
{
    // TODO - read projector resolution from config file

    class Framebuffer_window display_window(PROJECTOR_WIDTH, PROJECTOR_HEIGHT, "Projector display.", 18);
    if (display_window.error_status < 0)
    {
        std::cerr << "Failed to create projector display window.\n";
        return -1;
    }

    class Framebuffer_window preview_window(PROJECTOR_WIDTH, PROJECTOR_HEIGHT, "Preview.", 8);
    if (preview_window.error_status < 0)
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

    PNG png_image;

    const unsigned int num_commands = 1;
    //const struct command commands_array[num_commands] = {
    //    {QUIT, 4, "quit", 0, quit_command},
    //    {PREVIEW, 7, "preview", 1, preview_command}
    //};
    struct exposed_func commands_array[num_commands] = 
    {
        {"quit", 4, 0, {&window_data, NULL}, {}, quit_func},
        //{"preview", 7, 1, {&window_data, &png_image}, {{.arg_type = TEXT_BOOL, .text_bool = {"show", "hide"}}}, preview_func},
        //{"fill", 4, 2, {&preview_window}, {{.arg_type = CSV_UINT_LIST, .list_length = 3}, {.arg_type = ENUM, .enumeration_strings = blend_mode_strings}}, fill_func}
    };

    unsigned int command_idx, buffer_idx, length;
    char * command_buffer;
    while (!global_flags.close)
    {
        command_idx = 0;
        buffer_idx = 0;
        length = 0;
        if ((command_buffer = readline(">> ")) == NULL)
        {
            std::cerr << "Error: Received null pointer buffer.\n";
            return -1;
        }
        if (strlen(command_buffer) > 0)
        {
            add_history(command_buffer);
        }
        LENGTH_LOOP:
        switch (command_buffer[buffer_idx])
        {
            // Command names can only consist of alphabetics or underscore
            case 65 ... 90:
            case 97 ... 122:
            case 95:
            ++ buffer_idx;
            goto LENGTH_LOOP;

            default:
            break;
        }
        if (buffer_idx == 0)
        {
            std::cerr << "Error: No command name found.\n";
        }
        else
        {
            length = buffer_idx;
            for (command_idx = 0; command_idx < num_commands; command_idx ++)
            {
                if (commands_array[command_idx].name_length == length)
                {
                    if (commands_array[command_idx].name_string[0] == command_buffer[0])
                    {
                        while (-- length)
                        {
                            if (commands_array[command_idx].name_string[length] != command_buffer[length]) break;
                        }
                        if (length == 0)
                        {
                            if (commands_array[command_idx].num_args == 0)
                            {
                                commands_array[command_idx].func(&(commands_array[command_idx].generic_args), NULL);
                                break;
                            }
                            else
                            {
                                //if (handle_args((command_buffer + length), &(commands_array[command_idx])) < 0)
                                //{
                                //    std::cerr << "Error: Failed to interpret arguments to " << commands_array[command_idx].name_string << std::endl;
                                //}
                                break;
                            }
                        }
                    }
                }
            }
            if (command_idx == num_commands)
            {
                command_buffer[buffer_idx] = '\0';
                std::cerr << "Error: Unrecognised command " << command_buffer << std::endl;
            }
        }
        free(command_buffer);
    }

    pthread_join(window_thread_id, NULL);

    return 0;
}