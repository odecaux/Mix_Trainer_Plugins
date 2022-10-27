

// {-100, -12, -9, -6, -3};

#define ArraySize(array) (sizeof((array)) / sizeof(*(array)))

static double equal_double(double a, double b, double theta)
{
    return std::abs(a - b) < theta;
}

static int db_to_slider_pos(double db, const std::vector<double> &db_values)
{
    for(int i = 0; i < db_values.size(); i++)
    {
        if(equal_double(db, db_values[i], 0.001)) return i;
    }
    jassertfalse;
    return -1;
}

static int gain_to_slider_pos(double gain, const std::vector<double> &db_values)
{ 
    return db_to_slider_pos(juce::Decibels::gainToDecibels(gain), db_values);
}

static double slider_pos_to_gain(int pos, const std::vector<double> &db_values)
{
    return juce::Decibels::decibelsToGain(db_values[pos]);
}


enum GameStep {
    Begin,
    Listening,
    Editing,
    ShowingTruth,
    ShowingAnswer
};

static juce::String step_to_str(GameStep step)
{
    switch (step)
    {
        case Begin :
        {
            return "Begin";
        } break;
        case Listening :
        {
            return "Listening";
        } break;
        case Editing :
        {
            return "Editing";
        } break;
        case ShowingTruth :
        {
            return "ShowingTruth";
        } break;
        case ShowingAnswer :
        {
            return "ShowingAnswer";
        } break;
    }
}

struct ChannelState
{
    double edited_gain;
    double target_gain;
    //- unused 
    double target_low_shelf_gain;
    double target_low_shelf_freq;
    double target_high_shelf_gain;
    double target_high_shelf_freq;
    
    double edited_low_shelf_gain;
    double edited_low_shelf_freq;
    double edited_high_shelf_gain;
    double edited_high_shelf_freq;
};

struct ChannelInfos
{
    int id;
    juce::String name;
    float min_freq;
    float max_freq;
};

struct Game;

class ProcessorHost;
class Application;

struct GameUI;

struct Game {
    Application &app_old;
    GameUI *game_ui = nullptr;
    std::unordered_map<int, ChannelInfos> &channel_infos;
    std::function < void(const std::unordered_map < int, ChannelDSPState > &)> broadcastDSP;

    
    Game(Application &app, 
         std::unordered_map<int, ChannelInfos> &channel_infos, 
         std::function<void(const std::unordered_map < int, ChannelDSPState > &)> broadcastDSP) 
    : app_old(app), channel_infos(channel_infos), broadcastDSP(std::move(broadcastDSP))
    {}
    virtual ~Game() {}

    
    virtual void onChannelCreate(int id) {};
    virtual void onChannelDelete(int id) {};
    //virtual void onChannelRenamedFromUI(int id, const juce::String newName) {};
    virtual void onChannelRenamedFromTrack(int id, const juce::String &new_name) {}
    virtual void onChannelFrequenciesChanged(int id) {}

 

#if 0
    virtual int awardPoints() = 0;
    virtual void generateNewRound() = 0;
#endif
    virtual std::unique_ptr<GameUI> createUI() = 0;
    virtual void nextClicked() = 0;
    virtual void toggleInputOrTarget(bool clicked_was_target) = 0;
    
    virtual void finishUIInitialization() {}
    void onUIDelete();
};

struct GameUI_Panel;

struct GameUI : public juce::Component
{
    GameUI_Panel *panel = nullptr;
    void attachParentPanel(GameUI_Panel *new_panel)
    {
        jassert(panel == nullptr);
        jassert(new_panel != nullptr);
        panel = new_panel;
    }
    GameUI() {}
    virtual ~GameUI() {
        jassert(panel != nullptr);
    }
};


struct GameUI_Panel : public juce::Component
{
    GameUI_Panel(std::function < void() > && onNextClicked,
                 std::function < void(bool) > && onToggleClicked,
                 std::function < void() > && onBackClicked,
                 std::unique_ptr < GameUI > game_ui);
    
    //~GameUI_Panel() {}
    void updateGameUI_Generic(GameStep new_step, int new_score);
    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Label top_label;
    juce::Label score_label;
    juce::TextButton back_button;
            
    juce::TextButton next_button;
    juce::ToggleButton target_mix_button;
    juce::ToggleButton user_mix_button;

    std::unique_ptr < GameUI > game_ui;
};



struct ChannelNamesDemoUI : public GameUI
{
    ChannelNamesDemoUI(const std::unordered_map<int, ChannelInfos>& channel_infos,
                       std::function<void(int, bool)> onMuteToggle) :
        onMuteToggle(onMuteToggle)
    {
        auto f = 
            [this, &onToggle = this->onMuteToggle]
            (const auto &a) -> std::pair<int, std::unique_ptr<juce::ToggleButton>> {
            const int &id = a.first;
            auto new_toggle = std::make_unique < juce::ToggleButton > (a.second.name);
            new_toggle->onClick = [id = id, toggle = new_toggle.get(), &onToggle] {
                onToggle(id, toggle->getToggleState());
            };
            addAndMakeVisible(new_toggle.get());
            return { id, std::move(new_toggle)};
        };
        
        std::transform(channel_infos.begin(), channel_infos.end(), 
                        std::inserter(channel_toggles, channel_toggles.end()), 
                        f
        );
    }

    virtual ~ChannelNamesDemoUI() {}

    void resized() override {
        auto bounds = getLocalBounds();
        auto toggle_bounds = bounds.withHeight(50);
        for (auto &[id, button] : channel_toggles)
        {
            button->setBounds(toggle_bounds);
            toggle_bounds.translate(0, 50);
        }
    }

    //NOTE solution 1) keeping them in sync
    //solution 2) passing in the map and rebuilding the childs everytime

    void addChannel(int id, const juce::String name)
    {
        {
            auto assertChannel = channel_toggles.find(id);
            jassert(assertChannel == channel_toggles.end());
        }
        auto [it, result] = channel_toggles.emplace(id, std::make_unique<juce::ToggleButton>(name));
        jassert(result);
        auto &new_toggle = it->second;
        
        //new_toggle->setToggleState(playing, juce::dontSendNotification);
        new_toggle->onClick = [id = id, toggle = new_toggle.get(), &onToggle = this->onMuteToggle] {
            onToggle(id, toggle->getToggleState());
        };
        addAndMakeVisible(new_toggle.get());
        resized();
    }
    
    void removeChannel(int id)
    {
        const auto channel_to_remove = channel_toggles.find(id);
        jassert(channel_to_remove != channel_toggles.end());
        removeChildComponent(channel_to_remove->second.get());
        channel_toggles.erase(channel_to_remove);
        resized();
    }

    void changeChannelName(int id, const juce::String name)
    {
        channel_toggles[id]->setButtonText(name);
    }

    void updateGameUI(GameStep new_step, int new_score, std::unordered_map<int, bool> *mute_values_to_display)
    {
        if(mute_values_to_display)
            jassert(mute_values_to_display->size() == channel_toggles.size());

        for(auto& [id, toggle] : channel_toggles)
        {
            switch(new_step)
            {
                case Begin :
                {
                    jassert(mute_values_to_display != nullptr);
                    toggle->setEnabled(true);
                    toggle->setToggleState(mute_values_to_display->at(id), juce::dontSendNotification);
                } break;
                case Listening :
                {
                    jassert(mute_values_to_display == nullptr);
                    toggle->setEnabled(false);
                    toggle->setToggleState(false, juce::dontSendNotification);
                } break;
                case Editing :
                {
                    jassert(mute_values_to_display != nullptr);
                    toggle->setEnabled(true);
                    toggle->setToggleState(mute_values_to_display->at(id), juce::dontSendNotification);
                } break;
                case ShowingTruth :
                {
                    jassert(mute_values_to_display != nullptr);
                    toggle->setEnabled(false);
                    toggle->setToggleState(mute_values_to_display->at(id), juce::dontSendNotification);
                } break;
                case ShowingAnswer :
                {
                    jassert(mute_values_to_display != nullptr);
                    toggle->setEnabled(false);
                    toggle->setToggleState(mute_values_to_display->at(id), juce::dontSendNotification);
                } break;
            }
        }

        jassert(panel);
        panel->updateGameUI_Generic(new_step, new_score);
    }

    std::unordered_map < int, std::unique_ptr<juce::ToggleButton>> channel_toggles;
    std::function < void(int, bool) > onMuteToggle;
};


struct ChannelNamesDemo : public Game
{
    ChannelNamesDemo(Application &app, 
                     std::unordered_map<int, ChannelInfos> &channel_infos, 
                     std::function<void(const std::unordered_map < int, ChannelDSPState > &)> broadcastDSP) 
    : Game(app, channel_infos, std::move(broadcastDSP))
    {
        step = Begin;
        score = 0;
        std::transform(channel_infos.begin(), channel_infos.end(), 
                       std::inserter(edited_mutes, edited_mutes.end()), 
                       [](const auto &a) -> std::pair<int, bool> {return {a.first, true};});
        
        audioStateChanged();
        jassert(game_ui == nullptr);
    }
    virtual ~ChannelNamesDemo() {} 

    
    std::unordered_map < int, bool > *edit_or_target_RENAME()
    {
        if(step == Begin || step == Editing || step == ShowingAnswer)
            return &edited_mutes;
        else
            return &target_mutes;
    }

    void audioStateChanged() {
        auto * edit_or_target = edit_or_target_RENAME();

        std::unordered_map < int, ChannelDSPState > dsp;
        std::transform(edit_or_target->begin(), edit_or_target->end(), 
                        std::inserter(dsp, dsp.end()), 
                        [](const auto &a) -> std::pair<int, ChannelDSPState> {
                        if(a.second)
                            return {a.first, ChannelDSP_on()};
                        else 
                            return {a.first, ChannelDSP_off()};
                         
        });
        broadcastDSP(dsp);
    }
    
    std::unique_ptr < GameUI > createUI() override
    {
        jassert(game_ui == nullptr);
        auto new_ui = std::make_unique < ChannelNamesDemoUI > (
            channel_infos,
            [this](int id, bool is_toggled) {
                jassert(step == Begin || step == Editing);
                jassert(edited_mutes[id] != is_toggled);
                edited_mutes[id] = is_toggled;
                audioStateChanged();
            }
        );
        game_ui = new_ui.get();
        return new_ui;
    }
    
    void onChannelCreate(int id) override {
        auto [target, target_result] = target_mutes.emplace(id, true);
        jassert(target_result);
        auto [edited, edited_result] = edited_mutes.emplace(id, true);
        jassert(edited_result);
        
        if (auto *ui = getUI())
        {
              ui->addChannel(id, channel_infos[id].name);
        }

        audioStateChanged();
        uiChanged();
    }

    void onChannelDelete(int id) override {
        if (auto *ui = getUI())
        {
            ui->removeChannel(id);
        }
        {
            const auto channel_to_remove = edited_mutes.find(id);
            jassert(channel_to_remove != edited_mutes.end());
            edited_mutes.erase(channel_to_remove);
        }
        
        {
            const auto channel_to_remove = target_mutes.find(id);
            jassert(channel_to_remove != target_mutes.end());
            target_mutes.erase(channel_to_remove);
        }
        audioStateChanged();
    }
    
    void onChannelRenamedFromTrack(int id, const juce::String& new_name) override {
        if (auto *ui = getUI())
        {
            ui->changeChannelName(id, new_name);
        }
    }

    ChannelNamesDemoUI * getUI()
    {
        if (game_ui)
        {
            auto * ui = dynamic_cast<ChannelNamesDemoUI*>(game_ui);
            jassert(ui);
            return ui;
        }
        else
        {
            return nullptr;
        }
    }
    
    
    void toggleInputOrTarget(bool clicked_was_target) override {
        jassert(step != Begin);
        auto old_step = step;
        
        //TODO virtual affects score ?
        
        if(clicked_was_target && step == Editing)
        {
            step = Listening;
        }
        else if(clicked_was_target && step == ShowingAnswer)
        {
            step = ShowingTruth;
        }
        else if(!clicked_was_target && step == Listening)
        {
            step = Editing;
        }
        else if(!clicked_was_target && step == ShowingTruth){
            step = ShowingAnswer;
        }

        if(old_step != step)
        {

            audioStateChanged();
            uiChanged();
        }
    }

    void generateNewRound()
    {
        for (auto& [_, channel] : channel_infos)
        {
            target_mutes[channel.id] = juce::Random::getSystemRandom().nextBool();
            edited_mutes[channel.id] = true;
        }
        jassert(target_mutes.size() == channel_infos.size());
        jassert(edited_mutes.size() == channel_infos.size());
    }

    void nextClicked() override {

        switch(step)
        {
            case Begin :
            {
                jassert(target_mutes.size() == 0);
                generateNewRound();
                step = Listening;
            } break;
            case Listening :
            case Editing :
            {
                int points_awarded = 0;
                for (auto& [id, edited] : edited_mutes)
                {
                    if(edited == target_mutes[id])
                    {
                        points_awarded++;
                    }
                }
                score += points_awarded;
                step = ShowingTruth;
            }break;
            case ShowingTruth : 
            case ShowingAnswer :
            {
                generateNewRound();
                step = Listening;
            }break;
        }

        audioStateChanged();
        uiChanged();
    }

    void uiChanged()
    {
        if (auto *ui = getUI())
        {
            auto *mute_state_to_pass = step == Listening ? nullptr : edit_or_target_RENAME();
            ui->updateGameUI(step, score, mute_state_to_pass);
        }
    }

    void finishUIInitialization() override
    {
        jassert(game_ui);
        uiChanged();
    }
    
    GameStep step;
    int score;
    std::unordered_map < int, bool > edited_mutes;
    std::unordered_map < int, bool > target_mutes;
};




















class DecibelSlider : public juce::Slider
{
public:
    explicit DecibelSlider(const std::vector < double > &db_values)
    : db_values(db_values)
    {
    }
    
    juce::String getTextFromValue(double value) override
    {
        return juce::Decibels::toString(db_values[(int)value]);
    }

    const std::vector < double > &db_values;
};

enum FaderStep {
    FaderStep_Editing,
    FaderStep_Hiding,
    FaderStep_Showing
};

class FaderComponent : public juce::Component
{
public:
    explicit FaderComponent(const std::vector<double> &db_values,
                            const juce::String &name,
                            std::function<void(int)> &&onFaderMove,
                            std::function<void(const juce::String&)> &&onEditedName)
    :   onFaderMove(std::move(onFaderMove)),
        onEditedName(std::move(onEditedName)),
        fader(db_values)
    {
        label.setText(name, juce::NotificationType::dontSendNotification);
        label.setEditable(true);
        label.onTextChange = [this] {
            this->onEditedName(label.getText());
        };
        label.onEditorShow = [&] {
            auto *editor = label.getCurrentTextEditor();
            jassert(editor != nullptr);
            editor->setJustification(juce::Justification::centred);
        };
        addAndMakeVisible(label);
        
        fader.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxAbove, true, getWidth(), 20);
        fader.setRange(0.0f, (double)(db_values.size() - 1), 1.0);
        fader.setScrollWheelEnabled(true);
        
        fader.onValueChange = [this] {
            jassert(this->step == FaderStep_Editing);
            this->onFaderMove((int)fader.getValue());
        };
        addAndMakeVisible(fader);
    }
    
    
    void resized() override
    {
        auto r = getLocalBounds();
        auto labelBounds = r.withBottom(50);
        label.setBounds(labelBounds);
        
        auto faderBounds = r.withTrimmedTop(50);
        fader.setBounds(faderBounds);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(r.toFloat(), 5.0f, 2.0f);
    }
    
    void setName(const juce::String& new_name)
    {
        label.setText(new_name, juce::dontSendNotification);
    }
    
    void update(FaderStep new_step, int new_pos)
    {
        switch (new_step)
        {
            case FaderStep_Editing :
            {
                fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(true);
                fader.setVisible(true);
            } break;
            case FaderStep_Hiding :
            {
                jassert(new_pos == -1);
                //TODO ?
                //fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(false);
            } break;
            case FaderStep_Showing :
            {
                fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(true);
            } break;
        }

        step = new_step;
        repaint();
    }
    
private:
    juce::Label label;
    DecibelSlider fader;
    std::function<void(int)> onFaderMove;
    std::function<void(const juce::String&)> onEditedName;
    FaderStep step;
    //double targetValue;
    //double smoothing;
};

class FaderRowComponent : public juce::Component
{
public:
    FaderRowComponent(std::unordered_map<int, std::unique_ptr<FaderComponent>>& faders)
    : faders(faders)
    {
    }
    
    
    void adjustWidth() 
    {
        setSize(fader_width * (int)faders.size(), getHeight());
    }
    
    //==============================================================================
    
    void paint(juce::Graphics& g) override
    {
        juce::ignoreUnused(g);
    }
    
    
    void resized() override
    {
        int x_offset = 0;
        auto bounds = getLocalBounds();
        auto top_left = bounds.getTopLeft();
        //draw in order
        for (auto& [_, fader] : faders)
        {
            fader->setTopLeftPosition(top_left + juce::Point<int>(x_offset, 0));
            fader->setSize(fader_width, bounds.getHeight());
            x_offset += fader_width;
        }
        adjustWidth();
    }
    
private:
    const static int fader_width = 60;
    std::unordered_map<int, std::unique_ptr<FaderComponent>> &faders;
};

static FaderStep gameStepToFaderStep(GameStep game_step)
{
    switch (game_step)
    {
        case Begin :
        {
            return FaderStep_Editing;
        } break;
        case Listening :
        {
            return FaderStep_Hiding;
        } break;
        case Editing :
        {
            return FaderStep_Editing;
        } break; 
        case ShowingTruth :
        {
            return FaderStep_Showing;
        } break;
        case ShowingAnswer :
        {
            return FaderStep_Showing;
        } break;
    }
}





























struct MixerGameUI : public GameUI
{
    MixerGameUI(const std::unordered_map<int, ChannelInfos>& channel_infos,
                std::function<void(int, int)> onFaderMoved, //TODO lifetime ?
                const std::vector<double> &db_slider_values) :
        //TODO onEditedName
        onFaderMoved(onFaderMoved),
        fader_row(faders),
        db_slider_values(db_slider_values)
    {
        auto f = 
            [this, &onMoved = this->onFaderMoved] (const auto &a) -> std::pair<int, std::unique_ptr<FaderComponent>> {
            const int &id = a.first;
            
            auto onFaderMoved = [id = id, &onMoved] (int new_pos){
                onMoved(id, new_pos);
            };

            auto new_fader = std::make_unique < FaderComponent > (
                this->db_slider_values,
                a.second.name,
                std::move(onFaderMoved),
                [](auto ...){} //TODO onEditedName
            );
            fader_row.addAndMakeVisible(new_fader.get());
            return { id, std::move(new_fader)};
        };
        
        std::transform(channel_infos.begin(), channel_infos.end(), 
                       std::inserter(faders, faders.end()), 
                       f);
        fader_row.adjustWidth();

        addAndMakeVisible(fader_viewport);
        fader_viewport.setScrollBarsShown(false, true);
        fader_viewport.setViewedComponent(&fader_row, false);
    }

    virtual ~MixerGameUI() {}

    void resized() override {
        auto bounds = getLocalBounds();
        fader_viewport.setBounds(bounds);
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
        auto fader_moved = [&onMoved = this->onFaderMoved, id = id] (int new_pos){
            onMoved(id, new_pos);
        };
        auto [it, result] = faders.emplace(id, std::make_unique < FaderComponent > (
            db_slider_values,
            name,
            std::move(fader_moved),
            [](auto...){} //onEditedName
        ));
        jassert(result);
        auto &new_fader = it->second;
        //new_fader->setToggleState(playing, juce::dontSendNotification);

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

    void updateGameUI(GameStep new_step, int new_score, std::unordered_map<int, int > *slider_pos_to_display)
    {
        if(slider_pos_to_display)
            jassert(slider_pos_to_display->size() == faders.size());

        for(auto& [id, fader] : faders)
        {
            auto fader_step = gameStepToFaderStep(new_step);
            int pos = slider_pos_to_display ? slider_pos_to_display->at(id) : -1;
            fader->update(fader_step, pos);
        }

        jassert(panel);
        panel->updateGameUI_Generic(new_step, new_score);
    }

    std::unordered_map < int, std::unique_ptr<FaderComponent>> faders;
    FaderRowComponent fader_row;
    juce::Viewport fader_viewport;
    const std::vector < double > &db_slider_values;
    std::function<void(int, int)> onFaderMoved;
    std::function<void(int, const juce::String&)> onEditedName;
};


struct MixerGame : public Game
{
    MixerGame(Application &app, 
              std::unordered_map<int, ChannelInfos> &channel_infos, 
              std::vector<double> db_slider_values,
              std::function<void(const std::unordered_map < int, ChannelDSPState > &)> broadcastDSP) 
    : Game(app, channel_infos, std::move(broadcastDSP)),
        db_slider_values(std::move(db_slider_values))
    {
        step = Begin;
        score = 0;
        std::transform(channel_infos.begin(), channel_infos.end(), 
                       std::inserter(edited_slider_pos, edited_slider_pos.end()), 
                       [this](const auto &a) -> std::pair<int, int> {
                       return { a.first, (int)this->db_slider_values.size() - 2};
        });
        
        audioStateChanged();
        jassert(game_ui == nullptr);
    }
    virtual ~MixerGame() {} 

    
    std::unordered_map < int, int > *edit_or_target_RENAME()
    {
        if(step == Begin || step == Editing || step == ShowingAnswer)
            return &edited_slider_pos;
        else
            return &target_slider_pos;
    }

    void audioStateChanged() {
        auto * edit_or_target = edit_or_target_RENAME();

        std::unordered_map < int, ChannelDSPState > dsp;
        std::transform(edit_or_target->begin(), edit_or_target->end(), 
                       std::inserter(dsp, dsp.end()), 
                       [this](const auto &a) -> std::pair<int, ChannelDSPState> {
                       double gain = slider_pos_to_gain(a.second, db_slider_values);
                       return { a.first, ChannelDSP_gain(gain) };
                         
        });
        broadcastDSP(dsp);
    }
    
    std::unique_ptr < GameUI > createUI() override
    {
        jassert(game_ui == nullptr);
        auto new_ui = std::make_unique < MixerGameUI > (
            channel_infos,
            [this](int id, int pos) {
                jassert(step == Begin || step == Editing);
                //TODO assert sync ?
                edited_slider_pos[id] = pos;
                audioStateChanged(); 
            },
            db_slider_values
        );
        game_ui = new_ui.get();
        return new_ui;
    }
    
    void onChannelCreate(int id) override {
        auto [edited, edited_result] = edited_slider_pos.emplace(id, true);
        jassert(edited_result);
        auto [target, target_result] = target_slider_pos.emplace(id, true);
        jassert(target_result);
        
        if (auto *ui = getUI())
        {
            ui->addChannel(id, channel_infos[id].name);
        }

        audioStateChanged();
        uiChanged();
    }

    void onChannelDelete(int id) override {
        if (auto *ui = getUI())
        {
            ui->removeChannel(id);
        }
        {
            const auto channel_to_remove = edited_slider_pos.find(id);
            jassert(channel_to_remove != edited_slider_pos.end());
            edited_slider_pos.erase(channel_to_remove);
        }
        
        {
            const auto channel_to_remove = target_slider_pos.find(id);
            jassert(channel_to_remove != target_slider_pos.end());
            target_slider_pos.erase(channel_to_remove);
        }
        audioStateChanged();
    }
    
    void onChannelRenamedFromTrack(int id, const juce::String& new_name) override {
        if (auto *ui = getUI())
        {
            ui->changeChannelName(id, new_name);
        }
    }

    MixerGameUI * getUI()
    {
        if (game_ui)
        {
            auto * ui = dynamic_cast<MixerGameUI*>(game_ui);
            jassert(ui);
            return ui;
        }
        else
        {
            return nullptr;
        }
    }
    
    
    void toggleInputOrTarget(bool clicked_was_target) override {
        jassert(step != Begin);
        auto old_step = step;
        
        //TODO virtual affects score ?
        
        if(clicked_was_target && step == Editing)
        {
            step = Listening;
        }
        else if(clicked_was_target && step == ShowingAnswer)
        {
            step = ShowingTruth;
        }
        else if(!clicked_was_target && step == Listening)
        {
            step = Editing;
        }
        else if(!clicked_was_target && step == ShowingTruth){
            step = ShowingAnswer;
        }

        if(old_step != step)
        {

            audioStateChanged();
            uiChanged();
        }
    }

    void generateNewRound()
    {
        for (auto& [_, channel] : channel_infos)
        {
            target_slider_pos[channel.id] = juce::Random::getSystemRandom().nextInt() % db_slider_values.size();//;
            edited_slider_pos[channel.id] = (int)db_slider_values.size() - 2;//true;
        }
        jassert(target_slider_pos.size() == channel_infos.size());
        jassert(edited_slider_pos.size() == channel_infos.size());
    }

    void nextClicked() override {

        switch(step)
        {
            case Begin :
            {
                jassert(target_slider_pos.size() == 0);
                generateNewRound();
                step = Listening;
            } break;
            case Listening :
            case Editing :
            {
                int points_awarded = 0;
                for (auto& [id, edited] : edited_slider_pos)
                {
                    if(edited == target_slider_pos[id])
                    {
                        points_awarded++;
                    }
                }
                score += points_awarded;
                step = ShowingTruth;
            }break;
            case ShowingTruth : 
            case ShowingAnswer :
            {
                generateNewRound();
                step = Listening;
            }break;
        }

        audioStateChanged();
        uiChanged();
    }

    void uiChanged()
    {
        if (auto *ui = getUI())
        {
            auto *slider_pos_to_display = step == Listening ? nullptr : edit_or_target_RENAME();
            ui->updateGameUI(step, score, slider_pos_to_display);
        }
    }

    void finishUIInitialization() override
    {
        jassert(game_ui);
        uiChanged();
    }
    
    GameStep step;
    int score;
    std::unordered_map < int, int > edited_slider_pos;
    std::unordered_map < int, int > target_slider_pos;
    std::vector < double > db_slider_values;
};


#if 0
struct MixerGame : public Game {
    struct GameState {
        float edited_gain;
        float target_gain;
    };

    MixerGame(Application &app);
    virtual ~MixerGame() {} 

    ChannelState generateChannel(int id) override;
    int awardPoints() override;
    void generateNewRound() override;
    std::unique_ptr<GameUI> createUI() override;
    
    void editGain(int id, double new_gain);

    std::unordered_map < int, GameState > state;
};


struct MixerUI : public GameUI
{
    MixerUI(GameStep step,
            std::function<void()> &&onNextClicked,
            std::function<void(bool)> &&onToggleClicked,
            std::function<void()> &&onBackClicked,
            std::function<void(int, double)> &&onFaderMoved,
            std::function<void(int, const juce::String&)> && onEditedName);
    virtual ~MixerUI() {}
    
    void resized(juce::Rectangle<int> bounds) override;
    void createChannelUI(const ChannelInfos& channel, GameStep step) override;
    void deleteChannelUI(int id) override;
    void renameChannel(int id, const juce::String &newName) override;
    void updateGameUI(GameStep step) override;
    
    std::unordered_map<int, std::unique_ptr<FaderComponent>> faders = {};
    FaderRowComponent fader_row;
    juce::Viewport fader_viewport;
    std::function<void(int, double)> onFaderMoved;
    std::function<void(int, const juce::String&)> onEditedName;
};





#endif