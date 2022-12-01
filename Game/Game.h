#include <optional>


enum GameStep {
    GameStep_Begin,
    GameStep_Question,
    GameStep_Result,
    GameStep_EndResults,
    GameStep_None
};

enum Mix {
    Mix_User,
    Mix_Target,
    Mix_Hidden
};

FaderStep gameStepToFaderStep(GameStep game_step, Mix mix);

juce::String step_to_str(GameStep step);

struct ChannelInfos
{
    int id;
    juce::String name;
    float min_freq;
    float max_freq;
};

class ProcessorHost;
class Application;


enum Event_Type{
    Event_Init,
    Event_Click_Track,
    Event_Toggle_Track,
    Event_Slider, //value_i = slider_position
    Event_Click_Frequency,
    Event_Toggle_Input_Target, //value_b = was_target
    Event_Timeout,
    Event_Click_Begin,
    Event_Click_Done_Listening,
    Event_Click_Answer,
    Event_Click_Next,
    Event_Click_Back,
    Event_Click_Quit,
    Event_Channel_Create,
    Event_Channel_Delete,
    Event_Channel_Rename_From_Track,
    Event_Channel_Rename_From_UI,
    Event_Change_Frequency_Range,
    Event_Create_UI,
    Event_Destroy_UI
};

struct Event{
    Event_Type type;
    int id;
    bool value_b;
    int value_i;
    float value_f;
    float value_f_2;
    double value_d;
    char *value_cp;
    void *value_ptr;
    std::string value_str;
    juce::String value_js;
};

constexpr int game_ui_header_height = 20;

struct GameUI_Header : public juce::Component
{
    GameUI_Header();

    //void paint(juce::Graphics& g) override {} 
    void resized() override;

    juce::Label header_label;
    juce::Label score_label;
    juce::TextButton back_button;
            
    std::function<void()> onBackClicked;
};


struct GameUI_Bottom : public juce::Component
{
    GameUI_Bottom();

    //void paint(juce::Graphics& g) override {}
    void resized() override;

    juce::TextButton next_button;
    juce::ToggleButton target_mix_button;
    juce::ToggleButton user_mix_button;
    
    std::function<void()> onBeginClicked;
    std::function<void(Event_Type)> onNextClicked;
    std::function<void(bool)> onToggleClicked;

    Event next_button_event;
};

void game_ui_header_update(GameUI_Header *header, juce::String header_text, int new_score);
void game_ui_bottom_update(GameUI_Bottom *bottom, bool show_button, juce::String button_text, Mix mix, Event_Type event);
