#include <optional>

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

struct MixerGame_State {
    std::unordered_map<int, ChannelInfos> &channel_infos;
    GameStep step;
    int score;
    std::unordered_map < int, int > edited_slider_pos;
    std::unordered_map < int, int > target_slider_pos;
    //parametres
    int timeout_ms;
    std::vector < double > db_slider_values;
    Application *app;
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

struct MixerGameUI_2;

void mixer_game_post_event(MixerGame_State *state, Event event, MixerGameUI_2 *ui);



struct GameUI_Panel2 : public juce::Component
{
    GameUI_Panel2(juce::Component *game_ui);

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

struct MixerGameUI_2 : public juce::Component
{
    MixerGameUI_2(const std::unordered_map<int, ChannelInfos>& channel_infos,
                  const std::vector<double> &db_slider_values,
                  MixerGame_State *state) :
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
                mixer_game_post_event(state, event, ui);
            };
            
            auto onEdited = [id, state = state, ui](const juce::String & new_name){ 
                Event event = {
                    .type = Event_Channel_Rename_From_UI,
                    .id = id,
                    .value_js = new_name
                };
                mixer_game_post_event(state, event, ui);
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
            
            mixer_game_post_event(state, event, ui);
        };
        panel.onBackClicked = [state = state, ui = this] {
            Event event = {
                .type = Event_Click_Back
            };
            mixer_game_post_event(state, event, ui);
        };
        panel.onToggleClicked = [state = state, ui = this] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            mixer_game_post_event(state, event, ui);
        };
        addAndMakeVisible(panel);
    }

    virtual ~MixerGameUI_2() {}

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
            mixer_game_post_event(state, event, ui);
        };
            
        auto onEdited = [id, state = this->state, ui = this](const juce::String & new_name){ 
            Event event = {
                .type = Event_Channel_Rename_From_UI,
                .id = id,
                .value_js = new_name
            };
            mixer_game_post_event(state, event, ui);
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
            int in = slider_pos_to_display->size();
            int fa = faders.size();
            jassert(slider_pos_to_display->size() == faders.size());
            
        }
        for(auto& [id, fader] : faders)
        {
            auto fader_step = gameStepToFaderStep(new_step);
            int pos = slider_pos_to_display ? slider_pos_to_display->at(id) : -1;
            fader->update(fader_step, pos);
        }
        panel.update(new_step, new_score);
    }

    std::unordered_map < int, std::unique_ptr<FaderComponent>> faders;
    FaderRowComponent fader_row;
    juce::Viewport fader_viewport;
    const std::vector < double > &db_slider_values;
    GameUI_Panel2 panel;
    MixerGame_State *state;
};

static std::unique_ptr<MixerGame_State> mixer_game_init(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    int timeout_ms,
    std::vector<double> db_slider_values,
    Application *app)
{
    MixerGame_State state = {
        .channel_infos = channel_infos,
        .step = Begin,
        .score = 0,
        //.timeout_ms,
        .db_slider_values = db_slider_values,
        .app = app
    };
    std::transform(channel_infos.begin(), channel_infos.end(), 
                   std::inserter(state.edited_slider_pos, state.edited_slider_pos.end()), 
                   [&](const auto &a) -> std::pair<int, int> {
                   return { a.first, (int)db_slider_values.size() - 2};
    });
    return std::make_unique < MixerGame_State > (std::move(state));
}

static Effects mixer_game_update(MixerGame_State *state, Event event)
{
    GameStep old_step = state->step;
    GameStep step = old_step;
    Transition transition = Transition_None;
    bool update_audio = false;
    bool update_ui = false;

    Effects effects = { std::nullopt, std::nullopt, std::nullopt, false };

    switch (event.type)
    {
        case Event_Init : {
            jassertfalse;
            //update_audio = true;
            //update_ui = true;
        } break;
        case Event_Click_Track : {
            jassertfalse;
        } break;
        case Event_Toggle_Track : {
            jassertfalse;
        } break;
        case Event_Slider : {
            state->edited_slider_pos[event.id] = event.value_i;
            update_audio = true;
        } break;
        case Event_Toggle_Input_Target : {
            jassert(old_step != Begin);
            if(event.value_b && step == Editing)
            {
                step = Listening;
            }
            else if(event.value_b && step == ShowingAnswer)
            {
                step = ShowingTruth;
            }
            else if(!event.value_b && step == Listening)
            {
                step = Editing;
            }
            else if(!event.value_b && step == ShowingTruth){
                step = ShowingAnswer;
            }
            update_audio = true;
            update_ui = true;
        } break;
        case Event_Click_Begin : {
            jassert(step == Begin);
            jassert(state->target_slider_pos.size() == 0);
            transition = Transition_To_Exercice;
        } break;
        case Event_Click_Answer : {
            jassert(step == Listening || step == Editing);

            int points_awarded = 0;
            for (auto& [id, edited] : state->edited_slider_pos)
            {
                if(edited == state->target_slider_pos[id])
                {
                    points_awarded++;
                }
            }
            state->score += points_awarded;
            transition = Transition_To_Answer;
        } break;
        case Event_Click_Next : {
            transition = Transition_To_Exercice;
        } break;
        case Event_Click_Back : {
            effects.quit = true;
        } break;
        case Event_Click_Quit : {
            jassertfalse;
        } break;
        case Event_Channel_Create : {
            auto [edited, edited_result] = state->edited_slider_pos.emplace(event.id, true);
            jassert(edited_result);
            auto [target, target_result] = state->target_slider_pos.emplace(event.id, true);
            jassert(target_result);
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Channel_Delete : {
            {
                const auto channel_to_remove = state->edited_slider_pos.find(event.id);
                jassert(channel_to_remove != state->edited_slider_pos.end());
                state->edited_slider_pos.erase(channel_to_remove);
            }
        
            {
                const auto channel_to_remove = state->target_slider_pos.find(event.id);
                jassert(channel_to_remove != state->target_slider_pos.end());
                state->target_slider_pos.erase(channel_to_remove);
            }
            
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Channel_Rename_From_Track : {
            //update channel infos ?
        } break;
        case Event_Channel_Rename_From_UI : {
            effects.rename = std::make_optional < Effect_Rename > (event.id, event.value_str);
        } break;
        case Event_Change_Frequency_Range : {
            jassertfalse;
        } break;
        case Event_Create_UI :
        {
            update_ui = true;
        } break;
        case Event_Destroy_UI :
        {

        } break;
    }

    switch (transition)
    {
        case Transition_None : {
        }break;
        case Transition_To_Exercice : {

            for (auto& [_, channel] : state->channel_infos)
            {
                state->target_slider_pos[channel.id] = juce::Random::getSystemRandom().nextInt() % state->db_slider_values.size();//;
                state->edited_slider_pos[channel.id] = (int)state->db_slider_values.size() - 2;//true;
            }
            jassert(state->target_slider_pos.size() == state->channel_infos.size());
            jassert(state->edited_slider_pos.size() == state->channel_infos.size());

            step = Listening;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_Answer : {
            step = ShowingTruth;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_End_Result : {
        }break;
    }
    
    std::unordered_map<int, int>* edit_or_target;
    if(step == Begin || step == Editing || step == ShowingAnswer)
        edit_or_target = &state->edited_slider_pos;
    else
        edit_or_target = &state->target_slider_pos;

    if (update_audio)
    {
        std::unordered_map < int, ChannelDSPState > dsp;
        std::transform(edit_or_target->begin(), edit_or_target->end(), 
                       std::inserter(dsp, dsp.end()), 
                       [state](const auto &a) -> std::pair<int, ChannelDSPState> {
                       double gain = slider_pos_to_gain(a.second, state->db_slider_values);
                       return { a.first, ChannelDSP_gain(gain) };
        });
        effects.dsp = std::make_optional < Effect_DSP > (std::move(dsp));
    }

    if (update_ui)
    {
        std::optional < std::unordered_map<int, int> > slider_pos_to_display;
        
        if(step == Listening)
            slider_pos_to_display = std::nullopt; 
        else
            slider_pos_to_display = std::make_optional < std::unordered_map<int, int> > (*edit_or_target);

        effects.ui = std::make_optional< Effect_UI> (
            step,
            state->score,
            std::move(slider_pos_to_display)
        );
    }

    state->step = step;
    return effects;
}

//TODO mutex ? pour les timeout