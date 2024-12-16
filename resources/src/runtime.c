#include "runtime.h"
#include <dirent.h>
#include <lvgl/demos/lv_demos.h>
#include <lvgl/src/libs/tiny_ttf/lv_tiny_ttf.h>

#define SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR "SDL_EMSCRIPTEN_CANVAS_SELECTOR"

int lvrt_initialize(const char *canvas_selector){

  /* Initialize LVGL */

  lv_init();

  int monitor_hor_res = 320;
  int monitor_ver_res = 240;

  SDL_SetHint(SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR, canvas_selector);
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, canvas_selector);
  SDL_EventState(SDL_TEXTINPUT, SDL_DISABLE);
  SDL_EventState(SDL_KEYDOWN, SDL_DISABLE);
  SDL_EventState(SDL_KEYUP, SDL_DISABLE);

  lv_display_t *display = lv_sdl_window_create(monitor_hor_res, monitor_ver_res);
  lv_display_set_default(display);

  lv_sdl_window_set_title(display, "LVGL Editor");

  lv_group_t *group = lv_group_get_default();
  lv_group_set_default(lv_group_create());

  lv_indev_t * mouse = lv_sdl_mouse_create();
  lv_indev_set_group(mouse, group);

  lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
  lv_indev_set_group(mousewheel, group);

  /*lv_indev_t * keyboard = lv_sdl_keyboard_create();
  lv_indev_set_group(keyboard, group);*/
  
  lv_obj_t *screen = lv_display_get_screen_active(display);

  /*lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);

  lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_TRANSP, LV_PART_MAIN);

  lv_display_set_color_format(display, LV_COLOR_FORMAT_ARGB8888);*/

  lvrt_task_handler();

  emscripten_set_main_loop_arg(do_loop, NULL, 0, 0);

  return 0;

}

EMSCRIPTEN_KEEPALIVE
void do_loop(void* arg){
  lv_task_handler();
}

EMSCRIPTEN_KEEPALIVE
void lvrt_task_handler(){
  lv_task_handler();
}

EMSCRIPTEN_KEEPALIVE
int lvrt_process_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (file == NULL) {
    return 1;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  // Allocate memory and read file
  char *xml_content = (char *)malloc(size + 1);
  if (xml_content == NULL) {
    fclose(file);
    return 1;
  }

  fread(xml_content, 1, size, file);
  xml_content[size] = '\0';
  fclose(file);

  // Clean screen and create new object
  lv_obj_t *screen = lv_screen_active();
  lv_obj_clean(screen);
  
  int result = lvrt_process_data(xml_content);
  
  free(xml_content);
  
  if (result == 1) {
    return 1;
  }

  lvrt_task_handler();
  return 0;
}

EMSCRIPTEN_KEEPALIVE
int lvrt_process_data(const char *xml_definition){

  lv_obj_clean(lv_screen_active()); 

  lv_obj_t *screen = lv_screen_active();

  lv_xml_component_unregister("thisview");

  lv_result_t result = lv_xml_component_register_from_data("thisview", xml_definition);

  if(result != LV_RESULT_OK){
    LV_LOG_ERROR("Error processing data.");
    return 1;
  }

  /*const char * attrs[] = {
        "width", "320",
        "height", "240",
        NULL, NULL,
    };*/

  lv_obj_t * ui = lv_xml_create(screen, "thisview", NULL);
  
  if(ui == NULL){
    LV_LOG_ERROR("Ouch!");
    return 1;
  }
  
  lvrt_task_handler();

  return 0;

}

EMSCRIPTEN_KEEPALIVE
int lvrt_xml_load_component_data(const char *name, const char *xml_definition) {
  
  if(xml_definition==NULL){
    LV_LOG_ERROR("Cannot register %s. No data.", name);
    return 1;
  }

  lv_xml_component_register_from_data(name, xml_definition);

  return 0;
}

EMSCRIPTEN_KEEPALIVE
int lvrt_xml_load_component_file(const char *path) {
  if(path == NULL) {
    LV_LOG_ERROR("Cannot register component. No path.");
    return 1;
  }

  lv_xml_component_register_from_file(path);

  return 1;
}

void new_read_dir(const char * path)
{
    lv_fs_dir_t dir;
    lv_fs_res_t res;
    res = lv_fs_dir_open(&dir, path);
    if(res != LV_FS_RES_OK) {
      LV_LOG_ERROR("Failed to open directory %s", path);
      return;
    }

    char fn[256];
    while(1) {
        res = lv_fs_dir_read(&dir, fn, sizeof(fn));
        if(res != LV_FS_RES_OK) {
            LV_LOG_ERROR("Failed to read directory");
            break;
        }

        /* fn is empty, if not more files to read */
        if(strlen(fn) == 0) {
            break;
        }

        LV_LOG_USER("fn: %s", fn);
    }

    lv_fs_dir_close(&dir);
}

static int lvrt_register_resource(const char *name, const char *src_path, const unsigned char *data, size_t data_length, resource_type_t type, const resource_config_t* config) {
    const char *type_str = (type == RESOURCE_TYPE_IMAGE) ? "image" : "font";

    if (name == NULL || data == NULL || src_path == NULL) {
        LV_LOG_ERROR("Cannot register %s. Null values: name=%p, data=%p, path=%p", 
            type_str, (void*)name, (void*)data, (void*)src_path);
        return 1;
    }
    
    // Check if directory exists
    char dir_path[256];
    strcpy(dir_path, src_path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';  // Truncate at last slash to get directory path
        lv_fs_dir_t dir;
        if (lv_fs_dir_open(&dir, dir_path) != LV_FS_RES_OK) {
            LV_LOG_ERROR("Directory does not exist %s %s", dir_path, src_path);
            return 1;
        }
        lv_fs_dir_close(&dir);
    }
    
    lv_fs_file_t f;
    lv_fs_res_t res = lv_fs_open(&f, src_path, LV_FS_MODE_WR);
    if(res != LV_FS_RES_OK) {
        LV_LOG_ERROR("Failed to create %s file", type_str);
        return 1;
    }
    
    uint32_t bytes_written;
    res = lv_fs_write(&f, data, data_length, &bytes_written);
    lv_fs_close(&f);
    
    if(res != LV_FS_RES_OK || bytes_written != data_length) {
        LV_LOG_ERROR("Failed to write %s data (attempted: %zu, written: %u)", 
            type_str, data_length, bytes_written);
        return 1;
    }

    // Verify file was written by checking its size
    lv_fs_res_t verify_res = lv_fs_open(&f, src_path, LV_FS_MODE_RD);
    if (verify_res != LV_FS_RES_OK) {
        LV_LOG_ERROR("Failed to verify %s file", type_str);
        return 1;
    }
    
    uint32_t size;
    lv_fs_seek(&f, 0, LV_FS_SEEK_END);
    lv_fs_tell(&f, &size);
    lv_fs_close(&f);
    
    if (size == 0) {
        LV_LOG_ERROR("%s file is empty", type_str);
        return 1;
    }
    
    // Register based on type
    if (type == RESOURCE_TYPE_IMAGE) {
      lv_xml_register_image(name, src_path);
    } else {
        const lv_font_t * font = lv_tiny_ttf_create_file(src_path, config->font.font_size);
        if(font == NULL) {
            LV_LOG_ERROR("Failed to create font %s", src_path);
            return 1;
        }
        lv_xml_register_font(name, font);
    }

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int lvrt_register_image(const char *name, const char *src_path, 
                       const unsigned char *image_data, size_t data_length,
                       const char* file_format) {
    resource_config_t config = { .image = { .file_format = file_format } };
    return lvrt_register_resource(name, src_path, image_data, data_length, 
                                RESOURCE_TYPE_IMAGE, &config);
}

EMSCRIPTEN_KEEPALIVE
int lvrt_register_font(const char *name, const char *src_path, 
                      const unsigned char *font_data, size_t data_length,
                      int32_t font_size) {
    resource_config_t config = { .font = { .font_size = font_size } };
    return lvrt_register_resource(name, src_path, font_data, data_length, 
                                RESOURCE_TYPE_FONT, &config);
}

EMSCRIPTEN_KEEPALIVE
void lvrt_cleanup_runtime(){

}

void check_file(const char* filepath) {
    lv_fs_file_t f;
    lv_fs_res_t res = lv_fs_open(&f, filepath, LV_FS_MODE_RD);
    
    if(res == LV_FS_RES_OK) {
        uint32_t size;
        lv_fs_seek(&f, 0, LV_FS_SEEK_END);
        lv_fs_tell(&f, &size);
        LV_LOG_USER("File %s exists (%u bytes)", filepath, size);
        lv_fs_close(&f);
    } else {
        LV_LOG_ERROR("Failed to open file: %s", filepath);
    }
}
