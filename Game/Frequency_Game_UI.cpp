#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../Game/Game.h"
#include "../Game/Game_UI.h"
#include "Frequency_Game.h"
#include "Frequency_Widget.h"
#include "Frequency_Game_UI.h"

void frequency_game_ui_transitions(FrequencyGame_UI *ui, Effect_Transition transition, int ui_target)
{
    if (transition.out_transition == GameStep_Begin)
    {
        if (ui_target == 0)
        {
            auto frequency_widget = std::make_unique<FrequencyWidget>();
            frequency_widget->onClick = [io = ui->game_io] (uint32_t frequency) {
                Event event = {
                    .type = Event_Click_Frequency,
                    .value_u = frequency
                };
                frequency_game_post_event(io, event);
            };
            ui->center_panel = std::move(frequency_widget);
        }
        else
        {
            auto text_input = std::make_unique<Frequency_Game_Text_Input>();
            text_input->onClick = [io = ui->game_io] (uint32_t frequency) {
                Event event = {
                    .type = Event_Click_Frequency,
                    .value_u = frequency
                };
                frequency_game_post_event(io, event);
            };
            ui->center_panel = std::move(text_input);
        }
        ui->addAndMakeVisible(*ui->center_panel);
        ui->resized();
    }
    if (transition.in_transition == GameStep_EndResults)
    {
        assert(ui_target == 1);
        auto results_panel = std::make_unique < FrequencyGame_Results_Panel > ();
        ui->center_panel = std::move(results_panel);
        ui->addAndMakeVisible(*ui->center_panel);
        ui->resized();
    }
}

void frequency_game_ui_update(FrequencyGame_UI *ui, Frequency_Game_Effect_UI *new_ui)
{
    frequency_game_ui_transitions(ui, new_ui->transition, new_ui->ui_target);
    game_ui_header_update(&ui->header, new_ui->header_center_text, new_ui->header_right_text);
    if (ui->center_panel)
    {
        switch (new_ui->ui_target)
        {
            case 0 :
            {
                auto *frequency_widget = dynamic_cast<FrequencyWidget*>(ui->center_panel.get());
                assert(frequency_widget);
                frequency_widget_update(frequency_widget, new_ui);
            } break;
            case 1 :
            {
                auto *result_panel = dynamic_cast<FrequencyGame_Results_Panel*>(ui->center_panel.get());
                assert(result_panel);
                result_panel->score_label.setText("score : " + std::to_string(new_ui->results.score), juce::dontSendNotification);
            } break;
            case 2 :
            {
                auto *text_input = dynamic_cast<Frequency_Game_Text_Input*>(ui->center_panel.get());
                assert(text_input);
                text_input->text_input.setEnabled(!new_ui->freq_widget.is_cursor_locked);
                if (new_ui->freq_widget.display_target)
                {
                    text_input->text_input.setText(juce::String(new_ui->freq_widget.target_frequency));
                }
                if (new_ui->transition.in_transition == GameStep_Question)
                {
                    text_input->text_input.setText("");
                    text_input->text_input.grabKeyboardFocus();
                }
                if (new_ui->transition.out_transition == GameStep_Question)
                {
                    text_input->text_input.giveAwayKeyboardFocus();
                }
            };
        }
    }
    game_ui_bottom_update(&ui->bottom, new_ui->display_button, new_ui->button_text, new_ui->mix, new_ui->button_event);
}
