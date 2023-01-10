struct MixerGameUI;


struct Game_Mixer_Effect_Transition {
    GameStep in_transition;
    GameStep out_transition;
};

struct Game_Mixer_Effect_DSP {
    std::unordered_map<uint32_t, Channel_DSP_State> dsp_states;
};

struct Game_Mixer_Effect_UI {
    std::string header_center_text;
    std::string header_right_text;
    //int score; 
    std::optional < std::vector<uint32_t>> slider_pos_to_display;
    Widget_Interaction_Type widget_visibility;
    Mix mix_toggles;
    bool display_bottom_button;
    std::string bottom_button_text;
    Event_Type bottom_button_event;
};


enum MixerGame_Variant
{
    MixerGame_Normal,
    MixerGame_Tries,
    MixerGame_Timer
};

struct MixerGame_Config 
{
    std::string title;
    std::vector<Game_Channel> channel_infos;
    std::vector<double> db_slider_values;
    MixerGame_Variant variant;
    int listens;
    int timeout_ms;
};

struct MixerGame_State {
    GameStep step;
    Mix mix;
    int score;
    //mixer
    std::vector<uint32_t> edited_slider_pos;
    std::vector<uint32_t> target_slider_pos;
    
    MixerGame_Config config;
    int remaining_listens;
    bool can_still_listen;
    int64_t timestamp_start;
    int64_t current_timestamp;
};


struct Game_Mixer_Effects {
    int error;
    MixerGame_State new_state;
    std::optional < Game_Mixer_Effect_Transition > transition;
    std::optional < Game_Mixer_Effect_DSP > dsp;
    std::optional < Game_Mixer_Effect_UI > ui;
    bool quit;
};
using mixer_game_observer_t = std::function<void(const Game_Mixer_Effects &)>;

struct MixerGame_IO
{
    std::mutex mutex;
    Timer timer;
    std::vector<mixer_game_observer_t> observers;
    std::function<void()> on_quit;
    MixerGame_State game_state;
};


void mixer_game_post_event(MixerGame_IO *io, Event event);
Game_Mixer_Effects mixer_game_update(MixerGame_State state, Event event);
void mixer_game_add_observer(MixerGame_IO *io, mixer_game_observer_t new_observer);

MixerGame_State mixer_game_state_init(std::vector<Game_Channel> channel_infos,
                                      MixerGame_Variant variant,
                                      int listens,
                                      int timeout_ms,
                                      std::vector<double> db_slider_values);

std::unique_ptr<MixerGame_IO> mixer_game_io_init(MixerGame_State state);
