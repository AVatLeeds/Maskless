// Compile with g++ -Wall main.cpp XCB_framebuffer_window.cpp PNG_reader.cpp -o main.exec -lxcb -lxcb-image -lxcb-shm -lxcb-icccm -lreadline -lpng
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
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
    class Framebuffer_window * preview_window;
    class Framebuffer_window * display_window;
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
        struct text_bool text_bool;
        unsigned int list_length;
        char ** enumeration_strings;
        void * empty;
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

int png_load_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    uint8_t * temp_ptr;
    if (generic_args_ptr->image->open((char *)args_list[0]) < 0) return -1;
    for (unsigned int j = 0; j < generic_args_ptr->image->height(); j ++)
    {
        temp_ptr = generic_args_ptr->preview_window->framebuffer_ptr + (j * generic_args_ptr->preview_window->stride);
        // TODO get rid of magic number 3!
        for (unsigned int i = 0; i < generic_args_ptr->image->row_bytes(); i += 3)
        {
            *(temp_ptr ++) = generic_args_ptr->image->data_ptr[j][i + 0];
            *(temp_ptr ++) = generic_args_ptr->image->data_ptr[j][i + 1];
            *(temp_ptr ++) = generic_args_ptr->image->data_ptr[j][i + 2];
            *(temp_ptr ++) = 0;
        }
    }
    generic_args_ptr->preview_window->re_draw();
    return 0;
}

int clear_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    for (unsigned int i = 0; i < (generic_args_ptr->display_window->stride * generic_args_ptr->display_window->height); i ++)
    {
        generic_args_ptr->display_window->framebuffer_ptr[i] = 0;
    }
    generic_args_ptr->display_window->re_draw();
    return 0;
}

int align_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    clear_func(generic_args_ptr, NULL);
    uint8_t max_brightness = 128;
    unsigned int temp_for_scaling = 0;
    for (unsigned int i = 2; i < (generic_args_ptr->preview_window->stride * generic_args_ptr->preview_window->height); i += 4)
    {
        temp_for_scaling = generic_args_ptr->preview_window->framebuffer_ptr[i] * max_brightness;
        generic_args_ptr->display_window->framebuffer_ptr[i] = temp_for_scaling / 255;
    }
    generic_args_ptr->display_window->re_draw();
    return 0;
}

int expose_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    clear_func(generic_args_ptr, NULL);
    uint8_t max_brightness = 128;
    unsigned int temp_for_scaling = 0;
    for (unsigned int i = 0; i < (generic_args_ptr->preview_window->stride * generic_args_ptr->preview_window->height); i += 4)
    {
        temp_for_scaling = generic_args_ptr->preview_window->framebuffer_ptr[i] * max_brightness;
        generic_args_ptr->display_window->framebuffer_ptr[i + 0] = temp_for_scaling / 255;
        generic_args_ptr->display_window->framebuffer_ptr[i + 1] = temp_for_scaling / 255;
        generic_args_ptr->display_window->framebuffer_ptr[i + 2] = temp_for_scaling / 255;
    }
    generic_args_ptr->display_window->re_draw();
    return 0;
}

int display_fullscreen_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    generic_args_ptr->display_window->fullscreen();
    return 0;
}

int display_restore_func(struct generic_args * generic_args_ptr, void * args_list[])
{
    generic_args_ptr->display_window->restore();
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

int handle_args(char * command_buffer, struct exposed_func * func)
{
    void * args_list[func->num_args];
    char * temp_string_ptr;
    void * temp_data_ptr;
    int return_val;
    unsigned int arg_idx, i = 0;
    for (arg_idx = 0; arg_idx < func->num_args; arg_idx ++)
    {
        // absorb whitespace
        while ((*command_buffer == ' ') || (*command_buffer == '\t')) command_buffer ++;
        switch (func->args_list[arg_idx].arg_type)
        {
            case PATH:
            if (*command_buffer == '"')
            {
                temp_string_ptr = ++ command_buffer;
                while ((*command_buffer != '"') && (*command_buffer != '\0')) command_buffer ++;
                i = (command_buffer - temp_string_ptr);
                temp_string_ptr = (char *)malloc(sizeof(char) * i);
                if (*command_buffer == '"')
                {
                    temp_string_ptr[i --] = '\0';
                    for (; i; i --) temp_string_ptr[i] = *(-- command_buffer);
                    temp_string_ptr[0] = *(-- command_buffer);
                    std::cout << temp_string_ptr << std::endl;
                    args_list[arg_idx] = temp_string_ptr;
                }
                else
                {
                    std::cerr << "Error: No closing quote on path name.\n";
                    goto FAIL;
                }
            }
            else
            {
                std::cerr << "Error: No file path found.\n";
                std::cerr << "\tExpected \"path name\" as argument to " << func->name_string << std::endl;
                goto FAIL;
            }
            break;

            case TEXT_BOOL:
            /*
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
            */
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

    return_val = func->func(&(func->generic_args), args_list);
    for (arg_idx = 0; arg_idx < func->num_args; arg_idx ++) 
    {
        if (args_list[arg_idx] != NULL) free(args_list[arg_idx]);
    }
    return return_val;

    FAIL:
    for (arg_idx = 0; arg_idx < func->num_args; arg_idx ++)
    {
        if (args_list[arg_idx] != NULL) free(args_list[arg_idx]);
    }
    return -1;
}

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

    const class Framebuffer_window * window_class_ptr_array[] = {&display_window, &preview_window};
    const struct window_data window_data = {2, (class Framebuffer_window **)window_class_ptr_array};
    pthread_t window_thread_id;
    if (pthread_create(&window_thread_id, NULL, window_thread, (void *)(&window_data)) != 0)
    {
        std::cerr << "Error: Failed to create window handling thread.";
        return -1;
    }

    PNG png_image;

    const unsigned int num_commands = 7;
    //const struct command commands_array[num_commands] = {
    //    {QUIT, 4, "quit", 0, quit_command},
    //    {PREVIEW, 7, "preview", 1, preview_command}
    //};
    struct exposed_func commands_array[num_commands] = 
    {
        {"quit", 4, 0, {&preview_window, &display_window, NULL}, {{}}, quit_func},
        {"load_png", 8, 1, {&preview_window, &display_window, &png_image}, {{PATH, NULL}}, png_load_func},
        {"clear", 5, 0, {&preview_window, &display_window, NULL}, {{}}, clear_func},
        {"align", 5, 0, {&preview_window, &display_window, NULL}, {{}}, align_func},
        {"expose", 6, 0, {&preview_window, &display_window, NULL}, {{}}, expose_func},
        {"display_fullscreen", 18, 0, {&preview_window, &display_window, NULL}, {{}}, display_fullscreen_func},
        {"display_restore", 15, 0, {&preview_window, &display_window, NULL}, {{}}, display_restore_func}
        //{"preview", 7, 1, {&window_data, &png_image}, {{.arg_type = TEXT_BOOL, .text_bool = {"show", "hide"}}}, preview_func},
        //{"fill", 4, 2, {&preview_window}, {{.arg_type = CSV_UINT_LIST, .list_length = 3}, {.arg_type = ENUM, .enumeration_strings = blend_mode_strings}}, fill_func}
    };

    unsigned int command_idx, buffer_idx, length;
    char * command_buffer;
    while (global_flags.close == false)
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
                        while (-- buffer_idx)
                        {
                            if (commands_array[command_idx].name_string[buffer_idx] != command_buffer[buffer_idx]) break;
                        }
                        if (buffer_idx == 0)
                        {
                            if (commands_array[command_idx].num_args == 0)
                            {
                                commands_array[command_idx].func(&(commands_array[command_idx].generic_args), NULL);
                                break;
                            }
                            else
                            {
                                if (handle_args((command_buffer + length), &(commands_array[command_idx])) < 0)
                                {
                                    std::cerr << "Error: Command failed with arguments provided.\n";
                                }
                                break;
                            }
                        }
                    }
                }
            }
            if (command_idx == num_commands)
            {
                command_buffer[length] = '\0';
                std::cerr << "Error: Unrecognised command " << command_buffer << std::endl;
            }
        }
        free(command_buffer);
    }

    pthread_join(window_thread_id, NULL);

    std::cout << "Goodbye.\n";

    return 0;
}