struct MixerGameUI;


struct Effect_DSP {
    std::unordered_map<int, Channel_DSP_State> dsp_states;
};

struct Effect_UI {
    FaderStep fader_step; 
    juce::String header_text;
    int score; 
    std::optional < std::unordered_map<int, int> > slider_pos_to_display;
    juce::String button_text;
    Mix mix;
    Event_Type button_event;
};

struct Effect_Rename {
    int id;
    std::string new_name;
};

struct Effect_Timer {
    int timeout_ms;
    int gen_idx;
    std::function<void(int)> callback;
};

struct Effects {
    std::optional < Effect_DSP> dsp;
    std::optional < Effect_UI > ui;
    std::optional < Effect_Rename > rename;
    bool quit;
    std::optional < Effect_Timer > timer;
};

using audio_observer_t = std::function<void(Effect_DSP)>;
using ui_observer_t = std::function<void(Effect_UI &)>;

enum MixerGame_Variant
{
    MixerGame_Normal,
    MixerGame_Tries,
    MixerGame_Timer
};

struct MixerGame_State;

typedef Effects (*mixer_game_update_t)(MixerGame_State *, Event);

struct MixerGame_State {
    std::unordered_map<int, ChannelInfos> &channel_infos;
    //state
    GameStep step;
    Mix mix;
    int score;
    std::unordered_map < int, int > edited_slider_pos;
    std::unordered_map < int, int > target_slider_pos;
    //parametres
    MixerGame_Variant variant;
    int listens;
    int remaining_listens;
    bool can_still_listen;
    int timeout_ms;
    std::vector < double > db_slider_values;
    //io
    int gen_idx_active;
    int gen_idx_counter;
    std::unique_ptr<std::mutex> update_fn_mutex;
    Application *app;
    std::vector<ui_observer_t> observers_ui;
    Timer timer;
    std::vector<audio_observer_t> observers_audio;
};

void mixer_game_post_event(MixerGame_State *state, Event event);
Effects mixer_game_update(MixerGame_State *state, Event event);
void game_ui_update(Effect_UI &new_ui, MixerGameUI &ui);
void mixer_game_add_ui_observer(MixerGame_State *state, ui_observer_t && observer);
void mixer_game_add_audio_observer(MixerGame_State *state, audio_observer_t && observer);

struct MixerGameUI : public juce::Component
{
    MixerGameUI(const std::unordered_map<int, ChannelInfos>& channel_infos,
                const std::vector<double> &db_slider_values,
                MixerGame_State *state) 
        :
        fader_row(faders),
        db_slider_values(db_slider_values),
        state(state)
    {
        auto f = 
            [&] (const auto &a) -> std::pair<int, std::unique_ptr<FaderComponent>> {
            const int id = a.first;
            
            auto onFaderMoved = [state, id] (int new_pos){
                Event event = {
                    .type = Event_Slider,
                    .id = id,
                    .value_i = new_pos
                };
                mixer_game_post_event(state, event);
            };
            
            auto onEdited = [state, id](const juce::String & new_name){ 
                Event event = {
                    .type = Event_Channel_Rename_From_UI,
                    .id = id,
                    .value_js = new_name
                };
                mixer_game_post_event(state, event);
            };

            auto new_fader = std::make_unique < FaderComponent > (
                state->db_slider_values,
                a.second.name,
                std::move(onFaderMoved),
                std::move(onEdited)
            );
            fader_row.addAndMakeVisible(new_fader.get());
            return { id, std::move(new_fader)};
        };
        
        std::transform(channel_infos.begin(), channel_infos.end(), 
                       std::inserter(faders, faders.end()), 
                       f);
        fader_row.adjustWidth();
        fader_viewport.setScrollBarsShown(false, true);
        fader_viewport.setViewedComponent(&fader_row, false);

        bottom.onNextClicked = [state] (Event_Type e){
            Event event = {
                .type = e
            };
            
            mixer_game_post_event(state, event);
        };
        header.onBackClicked = [state] {
            Event event = {
                .type = Event_Click_Back
            };
            mixer_game_post_event(state, event);
        };
        bottom.onToggleClicked = [state] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            mixer_game_post_event(state, event);
        };
        addAndMakeVisible(header);
        addAndMakeVisible(fader_viewport);
        addAndMakeVisible(bottom);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds();
        auto bottom_height = 50;
        auto header_height = 20;

        auto header_bounds = bounds.withHeight(header_height);
        auto game_bounds = bounds.withTrimmedBottom(bottom_height).withTrimmedTop(header_height);
        auto bottom_bounds = bounds.withTrimmedTop(bounds.getHeight() - bottom_height);
        
        header.setBounds(header_bounds);
        fader_viewport.setBounds(game_bounds);
        bottom.setBounds(bottom_bounds);

        fader_row.setSize(fader_row.getWidth(),
                          fader_viewport.getHeight() - fader_viewport.getScrollBarThickness());
    }

    //NOTE solution 1) keeping them in sync
    //solution 2) passing in the map and rebuilding the childs everytime

    void addChannel(int id, const juce::String name)
    {
        {
            auto assertChannel = faders.find(id);
            assert(assertChannel == faders.end());
        }

        auto onFaderMoved = [state = this->state, id] (int new_pos){
            Event event = {
                .type = Event_Slider,
                .id = id,
                .value_i = new_pos
            };
            mixer_game_post_event(state, event);
        };
            
        auto onEdited = [state = this->state, id](const juce::String & new_name){ 
            Event event = {
                .type = Event_Channel_Rename_From_UI,
                .id = id,
                .value_js = new_name
            };
            mixer_game_post_event(state, event);
        };

        auto [it, result] = faders.emplace(id, std::make_unique < FaderComponent > (
            db_slider_values,
            name,
            std::move(onFaderMoved),
            std::move(onEdited)
        ));
        assert(result);
        auto &new_fader = it->second;

        fader_row.addAndMakeVisible(new_fader.get());
        fader_row.adjustWidth();
        resized();
    }
    
    void removeChannel(int id)
    {
        const auto channel_to_remove = faders.find(id);
        assert(channel_to_remove != faders.end());
        fader_row.removeChildComponent(channel_to_remove->second.get());
        faders.erase(channel_to_remove);
        fader_row.adjustWidth();
        resized();
    }

    void changeChannelName(int id, const juce::String name)
    {
        faders[id]->setName(name);
    }

    std::unordered_map < int, std::unique_ptr<FaderComponent>> faders;
    FaderRowComponent fader_row;
    juce::Viewport fader_viewport;
    const std::vector < double > &db_slider_values;
    GameUI_Header header;
    GameUI_Bottom bottom;
    MixerGame_State *state;
};

std::unique_ptr<MixerGame_State> mixer_game_init(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    MixerGame_Variant variant,
    int listens,
    int timeout_ms,
    std::vector<double> db_slider_values,
    Application *app);


//TODO mutex ? pour les timeout