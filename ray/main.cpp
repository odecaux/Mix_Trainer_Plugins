#include <juce_dsp/juce_dsp.h>
#include "../shared/shared.h"
#include "../Game/Game.h"
#include "../Game/Frequency_Game.h"
#include "miniaudio.h"
#include "raylib.h" 
#include "raygui.h"

#include <string.h>             // Required for: strcpy()
#include <assert.h>


#define DEVICE_FORMAT       ma_format_f32
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_waveform* pSineWave;
    
    assert(pDevice->playback.channels == DEVICE_CHANNELS);
    
    (void)pInput;   /* Unused. */
}

typedef enum {
    SCreen_Main,
    Screen_Frequency_Config,
    Screen_Compressor_Config,
    Screen_Audio_Files_Config
} Screen;

#define config_name_max_size 256
#define config_max_count 1024

typedef struct {
    char title[config_name_max_size];
    int mode_dropActive;
    int input_dropActive;
    float gain_sliderValue;
    float q_sliderValue;
    float initial_window_sliderValue;
    int initial_window_dropActive;
    float pre_listen_sliderValue;
    int pre_listen_dropActive;
    int question_dropActive;
    float question_sliderValue;
    float post_answer_sliderValue;
    bool post_answer_checkboxChecked;
} Frequency_Config;

char configs_text[config_max_count * config_name_max_size];

typedef struct {
    bool mode_dropEditMode;
    bool input_dropEditMode;
    bool initial_window_dropEditMode;
    bool pre_listen_dropEditMode;
    bool question_dropEditMode;
} Frequency_Config_Panel_State;

float row_height = 30;
int spacing = 12;

void Config_Panel(Frequency_Config_Panel_State *panel, Frequency_Config *config, int origin_x, int origin_y)
{
    if (panel->mode_dropEditMode || panel->input_dropEditMode || panel->initial_window_dropEditMode || panel->pre_listen_dropEditMode || panel->question_dropEditMode) 
        GuiLock();
    else 
        GuiUnlock();
    
    const char *mode_labelText = "Mode";
    const char *mode_dropText = "Normal;Bass";
    const char *input_labelText = "Input";
    const char *input_dropText = "Normal;Text";
    const char *gain_labelText = "Gain";
    const char *q_labelText = "Q";
    const char *initial_window_labelText = "Initial Window";
    const char *initial_window_dropText = "Free;Timeout;Rising Gain";
    const char *pre_listen_labelText = "Pre Listen";
    const char *pre_listen_dropText = "None;Timeout;Free";
    const char *question_labelText = "Question";
    const char *question_dropText = "Free;Timeout;Rising Gain";
    const char *post_answer_timeout_labelText = "Post Answer Timeout";
    
    int row = 0;
    float label_width = 120;
    float param_width = 70;
    float slider_width = 150;
    float value_width = 100;
    float label_x = origin_x;
    float param_x = origin_x + label_width;
    float slider_x = param_x + param_width + 15;
    float value_x = slider_x + slider_width + 15;
    float y = origin_y;
    
    char buf[256] = "";
    
    float mode_y = y;
    GuiLabel(Rectangle{ label_x, y, label_width, row_height }, mode_labelText);
    
    y += row_height + spacing;
    float input_y = y;
    GuiLabel(Rectangle{ label_x, y, label_width, row_height }, input_labelText);
    
    float gain_y = y;
    y += row_height + spacing;
    GuiLabel(Rectangle{ label_x, y, label_width, row_height }, gain_labelText);
    config->gain_sliderValue = GuiSlider(Rectangle{ slider_x, y, slider_width, row_height }, NULL, NULL, config->gain_sliderValue, 0, 100);
    GuiLabel(Rectangle{ value_x, y, value_width, row_height }, buf);
    
    y += row_height + spacing;
    float q_y = y;
    GuiLabel(Rectangle{ label_x, y, label_width, row_height }, q_labelText);
    config->q_sliderValue = GuiSlider(Rectangle{ slider_x, y, slider_width, row_height }, NULL, NULL, config->q_sliderValue, 0, 100);
    GuiLabel(Rectangle{ value_x, y, value_width, row_height }, buf);
    
    y += row_height + spacing;
    float initial_window_y = y;
    GuiLabel(Rectangle{ label_x, y, label_width, row_height }, initial_window_labelText);
    config->initial_window_sliderValue = GuiSlider(Rectangle{ slider_x, y, slider_width, row_height }, NULL, NULL, config->initial_window_sliderValue, 0, 100);
    GuiLabel(Rectangle{ value_x, y, value_width, row_height }, buf);
    
    y += row_height + spacing;
    float pre_listen_y = y;
    GuiLabel(Rectangle{ label_x, y, label_width, row_height }, pre_listen_labelText);
    config->pre_listen_sliderValue = GuiSlider(Rectangle{ slider_x, y, slider_width, row_height }, NULL, NULL, config->pre_listen_sliderValue, 0, 100);
    GuiLabel(Rectangle{ value_x, y, value_width, row_height }, buf);
    
    y += row_height + spacing;
    float question_y = y;
    GuiLabel(Rectangle{ label_x, y, label_width, row_height }, question_labelText);
    config->question_sliderValue = GuiSlider(Rectangle{ slider_x, y, slider_width, row_height }, NULL, NULL, config->question_sliderValue, 0, 100);
    GuiLabel(Rectangle{ value_x, y, value_width, row_height }, buf);
    
    y += row_height + spacing;
    float post_answer_y = y;
    GuiLabel(Rectangle{ label_x, y, label_width, row_height }, post_answer_timeout_labelText);
    config->post_answer_checkboxChecked = GuiCheckBox(Rectangle{ param_x, y, param_width, row_height }, NULL, config->post_answer_checkboxChecked);
    config->post_answer_sliderValue = GuiSlider(Rectangle{ slider_x, y, slider_width, row_height }, NULL, NULL, config->post_answer_sliderValue, 0, 100);
    GuiLabel(Rectangle{ value_x, y, value_width, row_height }, buf);
    
    
    if (GuiDropdownBox(Rectangle{ param_x, question_y, param_width, row_height }, question_dropText, &config->question_dropActive, panel->question_dropEditMode)) 
        panel->question_dropEditMode = !panel->question_dropEditMode;
    
    if (GuiDropdownBox(Rectangle{ param_x, pre_listen_y, param_width, row_height }, pre_listen_dropText, &config->pre_listen_dropActive, panel->pre_listen_dropEditMode)) 
        panel->pre_listen_dropEditMode = !panel->pre_listen_dropEditMode;
    
    if (GuiDropdownBox(Rectangle{ param_x, initial_window_y, param_width, row_height }, initial_window_dropText, &config->initial_window_dropActive, panel->initial_window_dropEditMode)) 
        panel->initial_window_dropEditMode = !panel->initial_window_dropEditMode;
    
    if (GuiDropdownBox(Rectangle{ slider_x, input_y, slider_width, row_height }, input_dropText, &config->input_dropActive, panel->input_dropEditMode)) 
        panel->input_dropEditMode = !panel->input_dropEditMode;
    
    if (GuiDropdownBox(Rectangle{ slider_x, mode_y, slider_width, row_height }, mode_dropText, &config->mode_dropActive, panel->mode_dropEditMode)) 
        panel->mode_dropEditMode = !panel->mode_dropEditMode;
}


int main()
{
    int screen_width = 800;
    int screen_height = 450;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screen_width, screen_height, "Mix Trainer");
    SetTargetFPS(60);
    
    //------------------------------------------------------------------------

    ma_device_config deviceConfig;
    ma_device device;
    
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = DEVICE_FORMAT;
    deviceConfig.playback.channels = DEVICE_CHANNELS;
    deviceConfig.sampleRate        = DEVICE_SAMPLE_RATE;
    deviceConfig.dataCallback      = data_callback;
    
    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return -4;
    }
    
    printf("Device Name: %s\n", device.playback.name);
    
    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        return -5;
    }

    //--------------------------------------------------------------------------------------
    
    Screen screen = SCreen_Main;
    
    int buttons_width = 120;
    int buttons_height = row_height;
    
    bool width_mode = false;
    bool height_mode = false;
    bool spacing_mode = false;
    
    int frequency_config_scroll_idx = 0;
    
    Frequency_Config *frequency_configs = (Frequency_Config*) malloc(sizeof(Frequency_Config) * config_max_count);
    int frequency_config_count = 3;
    int frequency_config_selected = -1;
    
    frequency_configs[0] = Frequency_Config { "one" };
    frequency_configs[1] = Frequency_Config { "two" };
    frequency_configs[2] = Frequency_Config { "three" };
    
    Frequency_Config_Panel_State config_panel = {
        false, false, false, false, false
    };
    
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        BeginDrawing();
        
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR))); 
        Vector2 center = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};
        
        if(screen == SCreen_Main)
        {
            float origin_x = center.x - (float)buttons_width / 2;
            float origin_y = center.y - (float)buttons_height / 2;
            Rectangle button_rects[3] = {
                { origin_x, origin_y - (float)buttons_height - spacing, (float)buttons_width, (float)buttons_height },
                { origin_x, origin_y + 0, (float)buttons_width, (float)buttons_height },
                { origin_x, origin_y + (float)buttons_height + spacing, (float)buttons_width, (float)buttons_height },
            };
            
            bool frequency_pressed = GuiButton(button_rects[0], "Frequency Game");
            bool compressor_pressed = GuiButton(button_rects[1], "Compressor");
            bool audio_files_pressed = GuiButton(button_rects[2], "Audio Files");
            
            if (frequency_pressed)
            {
                screen = Screen_Frequency_Config;
            }
            else if (compressor_pressed)
            {
                screen = Screen_Compressor_Config;
            }
            else if (audio_files_pressed)
            {
                screen = Screen_Audio_Files_Config;
            }
        }
        else if (screen == Screen_Frequency_Config)
        {
            
            configs_text[0] = 0;
            for (int i = 0; i < frequency_config_count; i++)
            {
                strcat(configs_text, frequency_configs[i].title);
                if(i != frequency_config_count - 1)
                    strcat(configs_text, ";");
            }
            Rectangle spinner_bounds = { 200, 40, 160, 350 };
            frequency_config_selected = GuiListView(spinner_bounds, configs_text, &frequency_config_scroll_idx, frequency_config_selected);
            
            {
                float small_width = 30;
                float origin_x = spinner_bounds.x + spinner_bounds.width + spacing;
                float origin_y = spinner_bounds.y;
                Rectangle button_rects[4] = {
                    { origin_x, origin_y + 0 * (small_width + spacing),small_width, small_width },
                    { origin_x, origin_y + 1 * (small_width + spacing), small_width, small_width },
                    { origin_x, origin_y + 2 * (small_width + spacing), small_width, small_width },
                    { origin_x, origin_y + 3 * (small_width + spacing), small_width, small_width },
                };
                
                bool add_pressed = GuiButton(button_rects[0], "add");
                bool remove_pressed = GuiButton(button_rects[1], "remove");
                bool up_pressed = GuiButton(button_rects[2], "up");
                bool down_pressed = GuiButton(button_rects[3], "down");
                
                if (add_pressed)
                {
                    frequency_configs[frequency_config_count++] = Frequency_Config { "new config" };
                }
                else if (remove_pressed
                         && frequency_config_selected != -1)
                {
                    for (int i = frequency_config_selected; i < frequency_config_count - 1; i++)
                    {
                        frequency_configs[i] = frequency_configs[i + 1];
                    }
                    frequency_config_count--;
                }
                else if (up_pressed
                         && frequency_config_selected != -1)
                {
                    if (frequency_config_selected != 0)
                    {
                        Frequency_Config temp = frequency_configs[frequency_config_selected - 1];
                        frequency_configs[frequency_config_selected - 1] = frequency_configs[frequency_config_selected];
                        frequency_configs[frequency_config_selected] = temp;
                        frequency_config_selected--;
                    }
                }
                else if (down_pressed
                         && frequency_config_selected != -1)
                {
                    if (frequency_config_selected != frequency_config_count - 1)
                    {
                        Frequency_Config temp = frequency_configs[frequency_config_selected + 1];
                        frequency_configs[frequency_config_selected + 1] = frequency_configs[frequency_config_selected];
                        frequency_configs[frequency_config_selected] = temp;
                        frequency_config_selected++;
                    }
                }
            }
            
            if (frequency_config_selected != -1)
            {
                float origin_y = spinner_bounds.y;
                float origin_x = spinner_bounds.x + spinner_bounds.width + 50;
                Config_Panel(&config_panel, &frequency_configs[frequency_config_selected], origin_x, origin_y);
            }
        }
        else if (screen == Screen_Compressor_Config)
        {
            
        }
        else if (screen == Screen_Audio_Files_Config)
        {
            
        }
        
        if (true)
        {
            Rectangle helper_rects[3] = {
                Rectangle { 80, 32, 120, row_height },
                Rectangle { 80, 64, 120, row_height },
                Rectangle { 80, 96, 120, row_height },
            };
            
            if (GuiSpinner(helper_rects[0], "Width", &buttons_width, 10, 1000, width_mode))
                width_mode = !width_mode;
            if (GuiSpinner(helper_rects[1], "Height", &buttons_height, 10, 1000, height_mode))
                height_mode = !height_mode;
            if (GuiSpinner(helper_rects[2], "Spacing", &spacing, 0, 1000, spacing_mode))
                spacing_mode = !spacing_mode;
        }
        
        GuiUnlock();
        EndDrawing();
    }
    
    ma_device_uninit(&device);
    CloseWindow();        // Close window and OpenGL context
    
    return 0;
}

