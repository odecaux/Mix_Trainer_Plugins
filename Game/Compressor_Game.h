

struct CompressorGame_Results
{
    int score;
    float analytics;
    //temps moyen ? distance moyenne ? stats ?
};

struct Compressor_Game_Effect_UI {
    Effect_Transition transition;
    CompressorGame_Results results;
    std::string header_center_text;
    std::string header_right_text;
    //int score;
    struct {
        Widget_Interaction_Type threshold_visibility;
        Widget_Interaction_Type ratio_visibility;
        Widget_Interaction_Type attack_visibility;
        Widget_Interaction_Type release_visibility;

        uint32_t threshold_pos;
        uint32_t ratio_pos;
        uint32_t attack_pos;
        uint32_t release_pos;
        
        std::vector < float > threshold_values_db;
        std::vector < float > ratio_values;
        std::vector < float > attack_values;
        std::vector < float > release_values;
    } comp_widget;
    Mix mix_toggles;
    std::string bottom_button_text;
    Event_Type bottom_button_event;
};

enum Compressor_Game_Variant
{
    Compressor_Game_Normal,
    Compressor_Game_Tries,
    Compressor_Game_Timer
};

struct CompressorGame_Config
{
    std::string title;
    //compressor
    bool threshold_active;
    bool ratio_active;
    bool attack_active;
    bool release_active;

    std::vector<float> threshold_values_db;
    std::vector<float> ratio_values;
    std::vector<float> attack_values;
    std::vector<float> release_values;
    //
    Compressor_Game_Variant variant;
    int listens;
    int timeout_ms;
    int total_rounds;
};

static void sort_clamp_and_filter(std::vector<float> *vec, float min, float max)
{
    std::sort(vec->begin(), vec->end());
    auto clamping = [min, max] (float in)
    {
        return std::clamp(in, min, max);
    };
    std::transform(vec->begin(), vec->end(), vec->begin(), clamping);
    vec->erase(std::unique(vec->begin(), vec->end()), vec->end());
}


static inline bool compressor_game_config_validate(CompressorGame_Config *config)
{
    sort_clamp_and_filter(&config->threshold_values_db, -90, 0);
    sort_clamp_and_filter(&config->ratio_values, 1, 10);
    sort_clamp_and_filter(&config->attack_values, 1, 1000);
    sort_clamp_and_filter(&config->release_values, 1, 1000);

    return config->threshold_active
        || config->ratio_active
        || config->attack_active
        || config->release_active;
}


struct CompressorGame_State
{
    GameStep step;
    Mix mix;
    int score;
    //compressor
    uint32_t target_threshold_pos;
    uint32_t target_ratio_pos;
    uint32_t target_attack_pos;
    uint32_t target_release_pos;

    uint32_t input_threshold_pos;
    uint32_t input_ratio_pos;
    uint32_t input_attack_pos;
    uint32_t input_release_pos;

    //
    uint32_t current_file_idx;
    std::vector<Audio_File> files;
    CompressorGame_Config config;
    CompressorGame_Results results;
    
    int remaining_listens;
    bool can_still_listen;
    int64_t timestamp_start;
    int64_t current_timestamp;
    int current_round;
};

struct Compressor_Game_Effects {
    int error;
    CompressorGame_State new_state;
    std::optional < Effect_Transition> transition;
    std::optional < Effect_DSP_Single_Track > dsp;
    std::optional < Effect_Player > player;
    std::optional < Compressor_Game_Effect_UI > ui;
    std::optional < CompressorGame_Results > results;
    bool quit;
};

using compressor_game_observer_t = std::function<void(Compressor_Game_Effects*)>;

struct CompressorGame_IO
{
    Timer timer;
    std::mutex update_fn_mutex;
    std::vector<compressor_game_observer_t> observers;
    std::function < void() > on_quit;
    CompressorGame_State game_state;
};


std::string compressor_game_serialize(std::vector<CompressorGame_Config> *compressor_game_configs);
std::vector<CompressorGame_Config> compressor_game_deserialize(std::string xml_string);

CompressorGame_Config compressor_game_config_default(std::string name);
CompressorGame_State compressor_game_state_init(CompressorGame_Config config, std::vector<Audio_File> *files);
std::unique_ptr<CompressorGame_IO> compressor_game_io_init(CompressorGame_State state);
void compressor_game_add_observer(CompressorGame_IO *io, compressor_game_observer_t observer);

void compressor_game_post_event(CompressorGame_IO *io, Event event);
Compressor_Game_Effects compressor_game_update(CompressorGame_State state, Event event);