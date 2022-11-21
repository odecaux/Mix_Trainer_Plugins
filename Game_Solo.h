using audio_observer_t = std::function<void(Effect_DSP)>;

struct SoloGameUI;

struct SoloGame_State {
    std::unordered_map<int, ChannelInfos> &channel_infos;
    //state
    GameStep step;
    int score;
    std::unordered_map < int, int > edited_slider_pos;
    std::unordered_map < int, int > target_slider_pos;
    //parametres
    std::vector < double > db_slider_values;
    //io
    Application *app;
    SoloGameUI *ui;
    std::vector<audio_observer_t> observers_audio;
};


void solo_game_post_event(SoloGame_State *state, Event event);
Effects solo_game_update(SoloGame_State *state, Event event);
void solo_game_ui_wrapper_update(GameUI_Wrapper *ui, GameStep new_step, int new_score);

static void solo_game_add_audio_observer(SoloGame_State *state, audio_observer_t observer)
{
    state->observers_audio.push_back(std::move(observer));
}

struct SoloGameUI : public juce::Component
{
    SoloGameUI(const std::unordered_map<int, ChannelInfos>& channel_infos,
                  const std::vector<double> &db_slider_values,
                  SoloGame_State *state) :
        fader_row(faders),
        db_slider_values(db_slider_values),
        panel(&fader_viewport),
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
                solo_game_post_event(state, event);
            };
            
            auto onEdited = [id, state = state, ui](const juce::String & new_name){ 
                Event event = {
                    .type = Event_Channel_Rename_From_UI,
                    .id = id,
                    .value_js = new_name
                };
                solo_game_post_event(state, event);
            };

            auto new_fader = std::make_unique < FaderComponent > (
                this->db_slider_values,
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

        panel.onNextClicked = [state = state, ui = this] (Event_Type e){
            Event event = {
                .type = e
            };
            
            solo_game_post_event(state, event);
        };
        panel.onBackClicked = [state = state, ui = this] {
            Event event = {
                .type = Event_Click_Back
            };
            solo_game_post_event(state, event);
        };
        panel.onToggleClicked = [state = state, ui = this] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            solo_game_post_event(state, event);
        };
        addAndMakeVisible(panel);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        panel.setBounds(bounds);
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
            solo_game_post_event(state, event);
        };
            
        auto onEdited = [id, state = this->state, ui = this](const juce::String & new_name){ 
            Event event = {
                .type = Event_Channel_Rename_From_UI,
                .id = id,
                .value_js = new_name
            };
            solo_game_post_event(state, event);
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

    void updateGameUI(GameStep new_step, int new_score, std::optional<std::unordered_map<int, int >> &slider_pos_to_display)
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
        solo_game_ui_wrapper_update(&panel, new_step, new_score);
    }

    std::unordered_map < int, std::unique_ptr<FaderComponent>> faders;
    FaderRowComponent fader_row;
    juce::Viewport fader_viewport;
    const std::vector < double > &db_slider_values;
    GameUI_Wrapper panel;
    SoloGame_State *state;
};

static std::unique_ptr<SoloGame_State> solo_game_init(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    std::vector<double> db_slider_values,
    Application *app)
{
    SoloGame_State state = {
        .channel_infos = channel_infos,
        .db_slider_values = db_slider_values,
        .app = app,
        .ui = nullptr
    };
    return std::make_unique < SoloGame_State > (std::move(state));
}