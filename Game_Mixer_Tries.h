using audio_observer_tries_t = std::function<void(Effect_DSP)>;

struct MixerGameUI_Tries;

struct MixerGame_State_Tries {
    std::unordered_map<int, ChannelInfos> &channel_infos;
    //state
    GameStep step;
    int score;
    std::unordered_map < int, int > edited_slider_pos;
    std::unordered_map < int, int > target_slider_pos;
    //parametres
    int listens;
    int remaining_listens;
    std::vector < double > db_slider_values;
    //io
    Application *app;
    MixerGameUI_Tries *ui;
    std::vector<audio_observer_tries_t> observers_audio;
};


void mixer_game_post_event_tries(MixerGame_State_Tries *state, Event event);
Effects mixer_game_tries_update(MixerGame_State_Tries *state, Event event);
void game_ui_wrapper_update_tries(GameUI_Top *top, GameUI_Bottom *bottom, GameStep new_step, int new_score, int remaining_listens);

static void mixer_game_tries_add_audio_observer(MixerGame_State_Tries *state, audio_observer_tries_t observer)
{
    state->observers_audio.push_back(std::move(observer));
}

struct MixerGameUI_Tries : public juce::Component
{
    MixerGameUI_Tries(const std::unordered_map<int, ChannelInfos>& channel_infos,
                  const std::vector<double> &db_slider_values,
                  MixerGame_State_Tries *state) :
        fader_row(faders),
        db_slider_values(db_slider_values),
        state(state)
    {
        auto f = 
            [this, state, ui = this] (const auto &a) -> std::pair<int, std::unique_ptr<FaderComponent>> {
            const int id = a.first;
            
            auto onFaderMoved = [id, state = state, ui] (int new_pos){
                Event event = {
                    .type = Event_Slider,
                    .id = id,
                    .value_i = new_pos
                };
                mixer_game_post_event_tries(state, event);
            };
            
            auto onEdited = [id, state = state, ui](const juce::String & new_name){ 
                Event event = {
                    .type = Event_Channel_Rename_From_UI,
                    .id = id,
                    .value_js = new_name
                };
                mixer_game_post_event_tries(state, event);
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

        bottom.onNextClicked = [state = state, ui = this] (Event_Type e){
            Event event = {
                .type = e
            };
            
            mixer_game_post_event_tries(state, event);
        };
        top.onBackClicked = [state = state, ui = this] {
            Event event = {
                .type = Event_Click_Back
            };
            mixer_game_post_event_tries(state, event);
        };
        bottom.onToggleClicked = [state = state, ui = this] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            mixer_game_post_event_tries(state, event);
        };
        addAndMakeVisible(top);
        addAndMakeVisible(fader_viewport);
        addAndMakeVisible(bottom);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds();
        auto bottom_height = 50;
        auto top_height = 20;
    
        auto top_bounds = bounds.withHeight(top_height);
        auto game_bounds = bounds.withTrimmedBottom(bottom_height).withTrimmedTop(top_height);
        auto bottom_bounds = bounds.withTrimmedTop(bounds.getHeight() - bottom_height);
        
        top.setBounds(top_bounds);
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
            jassert(assertChannel == faders.end());
        }

        auto onFaderMoved = [id, state = this->state, ui = this] (int new_pos){
            Event event = {
                .type = Event_Slider,
                .id = id,
                .value_i = new_pos
            };
            mixer_game_post_event_tries(state, event);
        };
            
        auto onEdited = [id, state = this->state, ui = this](const juce::String & new_name){ 
            Event event = {
                .type = Event_Channel_Rename_From_UI,
                .id = id,
                .value_js = new_name
            };
            mixer_game_post_event_tries(state, event);
        };

        auto [it, result] = faders.emplace(id, std::make_unique < FaderComponent > (
            db_slider_values,
            name,
            std::move(onFaderMoved),
            std::move(onEdited)
        ));
        jassert(result);
        auto &new_fader = it->second;

        fader_row.addAndMakeVisible(new_fader.get());
        fader_row.adjustWidth();
        resized();
    }
    
    void removeChannel(int id)
    {
        const auto channel_to_remove = faders.find(id);
        jassert(channel_to_remove != faders.end());
        fader_row.removeChildComponent(channel_to_remove->second.get());
        faders.erase(channel_to_remove);
        fader_row.adjustWidth();
        resized();
    }

    void changeChannelName(int id, const juce::String name)
    {
        faders[id]->setName(name);
    }

    void updateGameUI(GameStep new_step, int new_score, std::optional<std::unordered_map<int, int >> &slider_pos_to_display, int remaining_listens)
    {
        if (slider_pos_to_display)
        {
            jassert(slider_pos_to_display->size() == faders.size());
        }

        for(auto& [id, fader] : faders)
        {
            auto fader_step = gameStepToFaderStep(new_step);
            int pos = slider_pos_to_display ? slider_pos_to_display->at(id) : -1;
            fader->update(fader_step, pos);
        }
        game_ui_wrapper_update_tries(&top, &bottom, new_step, new_score, remaining_listens);
    }

    std::unordered_map < int, std::unique_ptr<FaderComponent>> faders;
    FaderRowComponent fader_row;
    juce::Viewport fader_viewport;
    const std::vector < double > &db_slider_values;
    GameUI_Top top;
    GameUI_Bottom bottom;
    MixerGame_State_Tries *state;
};

static std::unique_ptr<MixerGame_State_Tries> mixer_game_init_tries(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    int tries,
    std::vector<double> db_slider_values,
    Application *app)
{
    MixerGame_State_Tries state = {
        .channel_infos = channel_infos,
        .listens = tries,
        .db_slider_values = db_slider_values,
        .app = app,
        .ui = nullptr
    };
    return std::make_unique < MixerGame_State_Tries > (std::move(state));
}


//TODO mutex ? pour les timeout