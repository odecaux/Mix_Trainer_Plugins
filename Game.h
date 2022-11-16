#include <optional>


enum GameStep {
    Begin,
    Listening,
    Editing,
    ShowingTruth,
    ShowingAnswer
};

juce::String step_to_str(GameStep step);

#if 0
struct ChannelState
{
    double edited_gain;
    double target_gain;
    //- unused 
    double target_low_shelf_gain;
    double target_low_shelf_freq;
    double target_high_shelf_gain;
    double target_high_shelf_freq;
    
    double edited_low_shelf_gain;
    double edited_low_shelf_freq;
    double edited_high_shelf_gain;
    double edited_high_shelf_freq;
};
#endif

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
    Event_Toggle_Input_Target, //value_b = was_target
    Event_Click_Begin,
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
    std::string  value_str;
    juce::String value_js;
};

enum Transition {
    Transition_To_Exercice,
    Transition_To_Answer,
    Transition_To_End_Result,
    Transition_None,
};


struct Effect_DSP {
    std::unordered_map<int, ChannelDSPState> dsp_states;
};

struct Effect_UI {
    GameStep new_step; 
    int new_score; 
    std::optional < std::unordered_map<int, int> > slider_pos_to_display;
};

struct Effect_Rename {
    int id;
    std::string new_name;
};

struct Effects {
    std::optional < Effect_DSP> dsp;
    std::optional < Effect_UI > ui;
    std::optional < Effect_Rename > rename;
    bool quit;
};


struct GameUI_Wrapper : public juce::Component
{
    GameUI_Wrapper(juce::Component *game_ui);

    void update(GameStep new_step, int new_score);
    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Label top_label;
    juce::Label score_label;
    juce::TextButton back_button;
            
    juce::TextButton next_button;
    juce::ToggleButton target_mix_button;
    juce::ToggleButton user_mix_button;
    
    std::function<void()> onBeginClicked;
    std::function<void(Event_Type)> onNextClicked;
    std::function<void(bool)> onToggleClicked;
    std::function<void()> onBackClicked;

    juce::Component *game_ui;
    Event next_button_event;
};