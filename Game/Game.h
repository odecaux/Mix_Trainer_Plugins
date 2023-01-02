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
enum PreListen_Type
{
    PreListen_None = 0,
    PreListen_Timeout = 1,
    PreListen_Free = 2
};
struct Effect_Transition {
    GameStep in_transition;
    GameStep out_transition;
};

struct Effect_DSP_Single_Track {
    Channel_DSP_State dsp_state;
};

struct Effect_Player {
    std::vector<Audio_Command> commands;
};

Widget_Interaction_Type gameStepToFaderStep(GameStep game_step, Mix mix);

std::string step_to_str(GameStep step);



class ProcessorHost;
class Application;


enum Event_Type{
    Event_Init,
    Event_Click_Track,
    Event_Toggle_Track,
    Event_Slider, //value_i = slider_position
    Event_Click_Frequency,
    Event_Toggle_Input_Target, //value_b = was_target
    Event_Timer_Tick,
    Event_Click_Begin,
    Event_Click_Done_Listening,
    Event_Click_Answer,
    Event_Click_Next,
    Event_Click_Back,
    Event_Click_Quit,
    Event_Create_UI,
    Event_Destroy_UI
};

struct Event{
    Event_Type type;
    int id;
    int timer_gen_idx;
    bool value_b;
    int value_i;
    uint32_t value_u;
    juce::int64 value_i64;
    float value_f;
    float value_f_2;
    double value_d;
    char *value_cp;
    void *value_ptr;
    std::string value_str;
};
