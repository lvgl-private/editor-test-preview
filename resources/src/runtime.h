#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>

typedef enum {
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_FONT
} resource_type_t;

typedef struct {
    const char* file_format;
} image_config_t;

typedef struct {
    int32_t font_size;
} font_config_t;

typedef union {
    image_config_t image;
    font_config_t font;
} resource_config_t;

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <src/libs/expat/expat.h>
#include <lv_xml.h>
#include <lv_xml_component.h>
#include <lv_xml_widget.h>
//#include <lv_sdl_window.h>
#include <emscripten.h>
#include <SDL2/SDL.h>
#include <lvgl.h>
#include "lv_conf.h"
//#include "../components/parser/xml/lv_xml.h"

typedef struct {
    uint8_t* buffer;
    size_t size;
    int width;
    int height;
    int bytes_per_pixel;  
} screenshot_data_t;

typedef void (*display_refresh_cb_t)(lv_display_t* disp);

/* Function declarations */
int lvrt_initialize(const char *canvas_selector);
void do_loop(void* arg);
void lvrt_task_handler();
void lvrt_cleanup();
int lvrt_process_data(const char *data);
int lvrt_process_file(const char *path);
int lvrt_register_component(const char *name, const char *xml_definition);
void capture_sdl_screenshot(uint8_t* buffer);
//int lvrt_dom_tree_to_js(lv_dom_node_t *root_node);
void lvrt_xml_read_dir(const char * path);
void check_file(const char* filepath);
int lvrt_register_image(const char *name, const char *src_path, const unsigned char *image_data, size_t data_length, const char *file_format);
int lvrt_register_font(const char *name, const char *src_path, const unsigned char *font_data, size_t data_length, int32_t font_size);
void lvrt_print_obj_tree(lv_obj_t* obj, int level);

#endif /* RUNTIME_H */


