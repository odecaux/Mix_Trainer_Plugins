
struct GameUI_Header : public juce::Component
{
    GameUI_Header();

    //void paint(juce::Graphics& g) override {} 
    void resized() override;

    juce::Label center_label;
    juce::Label right_label;
    juce::TextButton back_button;
            
    std::function<void()> onBackClicked;
};


struct GameUI_Bottom : public juce::Component
{
    GameUI_Bottom();

    //void paint(juce::Graphics& g) override {}
    void resized() override;

    juce::TextButton next_button;
    juce::ToggleButton target_mix_button;
    juce::ToggleButton user_mix_button;
    
    //std::function<void()> onBeginClicked;
    std::function<void(Event_Type)> onNextClicked;
    std::function<void(bool)> onToggleClicked;

    Event next_button_event;
};

void game_ui_header_update(GameUI_Header *header, juce::String center_text, juce::String right_text);
void game_ui_bottom_update(GameUI_Bottom *bottom, bool show_button, juce::String button_text, Mix mix, Event_Type event);
