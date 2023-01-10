#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../shared/shared_ui.h"
#include "../Game/Game.h"
#include "../Game/Game_UI.h"
#include "../Game/Game_Mixer.h"
#include "../Game/Game_Mixer_UI.h"

void mixer_game_ui_transitions(MixerGameUI &ui, Game_Mixer_Effect_Transition transition)
{
    juce::ignoreUnused(ui);
    if (transition.out_transition == GameStep_Begin)
    {
    }
    if (transition.in_transition == GameStep_EndResults)
    {
    }
}

void game_ui_update(const Game_Mixer_Effect_UI &new_ui, MixerGameUI &ui)
{
    game_ui_header_update(&ui.header, new_ui.header_center_text, new_ui.header_right_text);
    if (new_ui.slider_pos_to_display)
    {
        assert(new_ui.slider_pos_to_display->size() == ui.faders.size());
    }
    for (auto i = 0; i < ui.faders.size(); i++)
    {
        int pos = new_ui.slider_pos_to_display ? new_ui.slider_pos_to_display->at(i) : -1;
        ui.faders[i]->update(new_ui.widget_visibility, pos);
    }
    game_ui_bottom_update(&ui.bottom, true, new_ui.bottom_button_text, new_ui.mix_toggles, new_ui.bottom_button_event);
}