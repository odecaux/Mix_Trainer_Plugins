#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../Game/Game.h"
#include "CompressorGame.h"
#include "Compressor_Game_UI.h"

void compressor_game_ui_transitions(CompressorGame_UI *ui, Effect_Transition transition)
{
    juce::ignoreUnused(ui);
    if (transition.in_transition == GameStep_Begin)
    {
        ui->previewer_file_list.setVisible(true);
        ui->resized();
#if 0  
        ui->previewer_file_list.selection_changed_callback = 
            [fileList, selection_changed = std::move(update_ui)] (const std::vector<bool> & new_selection)
        {
            fileList->selected = new_selection;
            selection_changed(new_selection);
        };
        std::vector<std::string> file_names{};
        for (uint32_t i = 0; fileList->files.size(); i++)
        {
            file_names.push_back(fileList->files[i].title);
        }
        list_comp.set_rows(file_names, fileList->selected);
#endif
    }
    if (transition.out_transition == GameStep_Begin)
    {
        ui->previewer_file_list.setVisible(false);
#if 0
        ui->compressor_widget = std::make_unique < CompressorWidget > ();
        ui->compressor_widget->onClick = [state = ui->game_state, io = ui->game_io] (int compressor) {
            Event event = {
                .type = Event_Click_Compressor,
                .value_u = compressor
            };
            compressor_game_post_event(state, io, event);
        };
        ui->addAndMakeVisible(*ui->compressor_widget);
#endif
        ui->resized();
    }
    if (transition.in_transition == GameStep_EndResults)
    {
#if 0
        ui->compressor_widget.reset();
        ui->results_panel = std::make_unique < CompressorGame_Results_Panel > ();
        ui->addAndMakeVisible(*ui->results_panel);
        ui->resized();
#endif
    }
}

void compressor_game_ui_update(CompressorGame_UI *ui, Compressor_Game_Effect_UI *new_ui)
{
    compressor_game_ui_transitions(ui, new_ui->transition);

	game_ui_header_update(&ui->header, new_ui->header_center_text, new_ui->header_right_text);
    
    auto threshold_text = [values = new_ui->comp_widget.threshold_values_db] (double new_pos) {
        return juce::Decibels::toString(values[static_cast<size_t>(new_pos)], 0);
    };

    auto ratio_text = [values = new_ui->comp_widget.ratio_values] (double new_pos) {
        return juce::String(values[static_cast<size_t>(new_pos)]);
    };

    auto attack_text = [values = new_ui->comp_widget.attack_values] (double new_pos) {
        return juce::String(values[static_cast<size_t>(new_pos)]) + " ms";
    };

    auto release_text = [values = new_ui->comp_widget.release_values] (double new_pos) {
        return juce::String(values[static_cast<size_t>(new_pos)]) + " ms";
    };

    using bundle_t = std::tuple<TextSlider&, Widget_Interaction_Type, uint32_t, uint32_t, std::function < juce::String(double)> >;

    auto bundle = std::vector<bundle_t>{
        { ui->compressor_widget.threshold_slider, new_ui->comp_widget.threshold_visibility, new_ui->comp_widget.threshold_pos, checked_cast<uint32_t>(new_ui->comp_widget.threshold_values_db.size()), std::move(threshold_text) },
        { ui->compressor_widget.ratio_slider, new_ui->comp_widget.ratio_visibility, new_ui->comp_widget.ratio_pos, checked_cast<uint32_t>(new_ui->comp_widget.ratio_values.size()), std::move(ratio_text) },
        { ui->compressor_widget.attack_slider, new_ui->comp_widget.attack_visibility,  new_ui->comp_widget.attack_pos, checked_cast<uint32_t>(new_ui->comp_widget.attack_values.size()), std::move(attack_text) },
        { ui->compressor_widget.release_slider, new_ui->comp_widget.release_visibility, new_ui->comp_widget.release_pos, checked_cast<uint32_t>(new_ui->comp_widget.release_values.size()), std::move(release_text) }
    };

    //TODO rename range
    for (auto &[slider, visibility, position, range, text_from_value] : bundle)
    {
        slider.setVisible(true);
        if (visibility != Widget_Hiding)
        {
            slider.get_text_from_value = std::move(text_from_value);
            slider.setRange(0.0, (double)(range - 1), 1.0);
            slider.setValue(static_cast<double>(position), juce::dontSendNotification);
        }
        else
        {
            slider.get_text_from_value = [](auto ...) { return ""; };
            slider.setRange(0.0, 1.0, 1.0);
            slider.setValue(0.0, juce::dontSendNotification);
        }

        switch (visibility)
        {
            case Widget_Editing :
            {
                slider.setEnabled(true);
            } break;
            case Widget_Hiding :
            case Widget_Showing :
            {
                slider.setEnabled(false);
            } break;
        }
    }

    game_ui_bottom_update(&ui->bottom, true, new_ui->bottom_button_text, new_ui->mix_toggles, new_ui->bottom_button_event);
}