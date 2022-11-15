enum Event_Type{
    Event_Init,
    Event_Click_Track,
    Event_Toggle_Track,
    Event_Slider, //value_i = slider_position
    Event_Toggle_Input_Target, //value_b = was_target
    Event_Click_Begin,
    Event_Click_Answer,
    Event_Click_Next,
    Event_Click_Quit,
    Event_Channel_Create,
    Event_Channel_Delete,
    Event_Channel_Rename_From_Track,
    Event_Channel_Rename_From_UI,
};

struct Event{
    Event_Type type;
    int id;
    bool value_b;
    int value_i;
    float value_f;
    double value_d;
    char *value_s;
};

enum Transition {
    Transition_To_Exercice,
    Transition_To_Answer,
    Transition_To_End_Result,
    Transition_None,
};

struct MixerGame_State {
    std::unordered_map<int, ChannelInfos> channel_infos;
    GameStep step;
    int score;
    std::unordered_map < int, int > edited_slider_pos;
    std::unordered_map < int, int > target_slider_pos;
    //parametres
    int timeout_ms;
    std::vector < double > db_slider_values;
};

MixerGame_State mixer_game_init(
    std::unordered_map<int, ChannelInfos> channel_infos,
    int timeout_ms,
    std::vector < double > db_slider_values)
{
    MixerGame_State state = {
        .channel_infos = channel_infos,
        .step = Begin,
        .score = 0,
        //.timeout_ms,
        .db_slider_values = db_slider_values
    };
}

struct DSP_Effect {
    std::unordered_map<int, ChannelDSPState> dsp_states;
};

struct UI_Effect {
    GameStep new_step; 
    int new_score; 
    std::unique_ptr < std::unordered_map<int, int> > slider_pos_to_display;
};

struct Effects {
    std::unique_ptr < DSP_Effect> dsp;
    std::unique_ptr < UI_Effect > ui;
};

Effects mixer_game_update(MixerGame_State *state, Event event)
{
    GameStep old_step = state->step;
    GameStep step = old_step;
    Transition transition = Transition_None;
    bool update_audio = false;
    bool update_ui = false;

    switch (event.type)
    {
        case Event_Init : {
            update_audio = true;
            update_ui = true;
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
        case Event_Click_Quit : {

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
            this->onEditedName(event.id, event.value_s);
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
    
    Effects effects = { nullptr, nullptr };
    
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
                       [this](const auto &a) -> std::pair<int, ChannelDSPState> {
                       double gain = slider_pos_to_gain(a.second, db_slider_values);
                       return { a.first, ChannelDSP_gain(gain) };
                         
        });
        effects.dsp = std::make_unique < DSP_Effect > (std::move(dsp));
    }

    if (update_ui)
    {
        std::unique_ptr < std::unordered_map<int, int> > slider_pos_to_display;
        
        if(step == Listening)
            slider_pos_to_display = nullptr; 
        else 
            slider_pos_to_display = std::make_unique< std::unordered_map<int, int>>(*edit_or_target);

        effects.ui = std::make_unique< UI_Effect> (
            step,
            state->score,
            std::move(slider_pos_to_display)
        );
    }

    state->step = step;
    return effects;
}



void mixer_game_post_event(MixerGame_State *state, Event event)
{
    Effects effects = mixer_game_update(state, event);
    if (effects.dsp)
    {

    }
    if (effects.ui)
    {
        //if ui exists ?
    }
}




//TODO mutex ? pour les timeout