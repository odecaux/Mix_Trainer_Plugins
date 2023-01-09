struct FrequencyGame_Results_Panel : public juce::Component
{
    FrequencyGame_Results_Panel()
    {
        score_label.setSize(100, 50);
        addAndMakeVisible(score_label);
    }

    void resized()
    {
        auto r = getLocalBounds();
        score_label.setCentrePosition(r.getCentre());
    }
    juce::Label score_label;
};

struct FrequencyGame_Results
{
    int score;
    float analytics;
    //temps moyen ? distance moyenne ? stats ?
};


struct Frequency_Game_Effect_UI {
    Effect_Transition transition;
    int ui_target;
    struct {
        bool display_target;
        uint32_t target_frequency;

        bool is_cursor_locked;
        uint32_t locked_cursor_frequency;
        
        bool display_window;
        float correct_answer_window;
        
        uint32_t min_f;
        uint32_t num_octaves;
    } freq_widget;
    FrequencyGame_Results results;
    std::string header_center_text;
    std::string header_right_text;
    Mix mix;
    bool display_button;
    std::string button_text;
    Event_Type button_event;
};


enum Frequency_Input
{
    Frequency_Input_Widget,
    Frequency_Input_Text
};

enum Frequency_Question_Type
{
    Frequency_Question_Free = 0,
    Frequency_Question_Timeout = 1,
    Frequency_Question_Rising = 2
};

struct FrequencyGame_Config
{
    std::string title;
    //frequency
    Frequency_Input input;
    float eq_gain_db;
    float eq_quality;
    float initial_correct_answer_window;
    uint32_t min_f;
    uint32_t num_octaves;
    //
    PreListen_Type prelisten_type;
    int prelisten_timeout_ms;

    Frequency_Question_Type question_type;
    int question_timeout_ms;

    bool result_timeout_enabled;
    int result_timeout_ms;
};

struct FrequencyGame_State
{
    GameStep step;
    int score;
    int lives;
    //frequency
    uint32_t target_frequency;
    float correct_answer_window; 
    bool is_prelistening;
    //
    uint32_t current_file_idx;
    std::vector<Audio_File> files;
    FrequencyGame_Config config;
    FrequencyGame_Results results;

    int64_t timestamp_start;
    int64_t current_timestamp;
};

struct Frequency_Game_Effects {
    int error;
    std::optional < Effect_Transition> transition;
    std::optional < Effect_DSP_Single_Track > dsp;
    std::optional < Effect_Player > player;
    std::optional < Frequency_Game_Effect_UI > ui;
    std::optional < FrequencyGame_Results > results;
    bool quit;
    FrequencyGame_State new_state;
};

using frequency_game_observer_t = std::function<void(Frequency_Game_Effects*)>;

struct FrequencyGame_IO
{
    FrequencyGame_State game_state;
    Timer timer;
    std::mutex update_fn_mutex;
    std::vector<frequency_game_observer_t> observers;
    std::function < void() > on_quit;
};


struct FrequencyGame_UI;

std::string frequency_game_serlialize(std::vector<FrequencyGame_Config> *frequency_game_configs);
std::vector<FrequencyGame_Config> frequency_game_deserialize(std::string xml_string);


FrequencyGame_Config frequency_game_config_default(std::string name);
FrequencyGame_State frequency_game_state_init(FrequencyGame_Config config, std::vector<Audio_File> *files);
std::unique_ptr<FrequencyGame_IO> frequency_game_io_init(FrequencyGame_State);
void frequency_game_add_observer(FrequencyGame_IO *io, frequency_game_observer_t observer);

void frequency_game_post_event(FrequencyGame_IO *io, Event event);
Frequency_Game_Effects frequency_game_update(FrequencyGame_State state, Event event);
