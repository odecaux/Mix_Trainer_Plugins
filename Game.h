
static const double slider_values[] = {-100, -12, -9, -6, -3};

#define ArraySize(array) (sizeof((array)) / sizeof(*(array)))

static double equal_double(double a, double b, double theta)
{
    return std::abs(a - b) < theta;
}

static double slider_value_to_db(int value)
{
    jassert(value < ArraySize(slider_values));
    return slider_values[value];
}


static int db_to_slider_value(double db)
{
    for(int i = 0; i < ArraySize(slider_values); i++)
    {
        if(equal_double(db, slider_values[i], 0.001)) return i;
    }
    jassertfalse;
    return -1;
}

static int gain_to_slider_value(double gain)
{ 
    return db_to_slider_value(juce::Decibels::gainToDecibels(gain));
}

static double slider_value_to_gain(int value)
{
    return juce::Decibels::decibelsToGain(slider_value_to_db(value));
}

enum Action {
    
};

enum GameStep {
    Begin,
    Listening,
    Editing,
    ShowingTruth,
    ShowingAnswer
};

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
    //GameStep step;
    //int score;
    
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
    virtual ChannelState generateChannel(int id) = 0;
    virtual int awardPoints() = 0;
    virtual void generateNewRound() = 0;
#endif
    virtual std::unique_ptr<GameUI> createUI() = 0;

    void onUIDelete();
    
    void toggleInputOrTarget(bool isOn);
    void nextClicked();
    void backClicked();
};


struct GameUI : public juce::Component
{
    GameUI() {}
    virtual ~GameUI() {}

#if 0
    virtual void createChannelUI(const ChannelInfos& channel_infos, GameStep step) = 0;
    virtual void deleteChannelUI(int id) = 0;
    virtual void renameChannel(int id, const juce::String &newName) = 0;
    //virtual void updateGameUI(const GameState &state) = 0;
#endif
};


struct GameUI_Panel : public juce::Component
{
    GameUI_Panel(std::function < void() > && onNextClicked,
                 std::function < void(bool) > && onToggleClicked,
                 std::function < void() > && onBackClicked,
                 std::unique_ptr < GameUI > game_ui);
    
    //~GameUI_Panel() {}
    //void updateGameUI_Generic(const GameState &state);
    void paint(juce::Graphics& g) override;
    void resized() override;

#if 0
    virtual void resizedChild(juce::Rectangle<int> bounds) = 0;
    virtual void createChannelUI(const ChannelInfos& channel_infos, GameStep step) = 0;
    virtual void deleteChannelUI(int id) = 0;
    virtual void renameChannel(int id, const juce::String &newName) = 0;
    //virtual void updateGameUI(const GameState &state) = 0;
#endif
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
                       const std::unordered_map<int, bool> &mute_state,
                       std::function<void(int, bool)> onMuteToggle) :
        onMuteToggle(onMuteToggle)
    {
        auto f = 
            [this, &onToggle = this->onMuteToggle, &mute_state]
            (const auto &a) -> std::pair<int, std::unique_ptr<juce::ToggleButton>> {
            const int &id = a.first;
            auto new_toggle = std::make_unique < juce::ToggleButton > (a.second.name);
            bool is_playing = mute_state.at(id);
            new_toggle->setToggleState(is_playing, juce::dontSendNotification);
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
        for (const auto& [id, is_toggled] : mute_state) {
            channel_toggles[id]->setToggleState(is_toggled, juce::dontSendNotification);
        }

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

    void addChannel(int id, const juce::String name, bool playing)
    {
        {
            auto assertChannel = channel_toggles.find(id);
            jassert(assertChannel == channel_toggles.end());
        }
        auto [it, result] = channel_toggles.emplace(id, std::make_unique<juce::ToggleButton>(name));
        jassert(result);
        auto &new_toggle = it->second;
        new_toggle->setToggleState(playing, juce::dontSendNotification);
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
        std::transform(channel_infos.begin(), channel_infos.end(), 
                       std::inserter(channel_mute, channel_mute.end()), 
                       [](const auto &a) -> std::pair<int, bool> {return {a.first, true};});

        audioStateChanged();
    }
    virtual ~ChannelNamesDemo() {} 

    void audioStateChanged() {
        std::unordered_map < int, ChannelDSPState > dsp;
        std::transform(channel_mute.begin(), channel_mute.end(), 
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
            channel_mute,
            [this](int id, bool is_toggled) {
                jassert(channel_mute[id] != is_toggled);
                channel_mute[id] = is_toggled;
                audioStateChanged();
            }
        );
        game_ui = new_ui.get();
        return new_ui;
    }
    
    void onChannelCreate(int id) override {
        auto [it, result] = channel_mute.emplace(id, true);
        jassert(result);

        if (auto *ui = getUI())
        {
            ui->addChannel(id, channel_infos[id].name, it->second);
        }
        audioStateChanged();
    }
    void onChannelDelete(int id) override {
        if (auto *ui = getUI())
        {
            ui->removeChannel(id);
        }
        const auto channel_to_remove = channel_mute.find(id);
        jassert(channel_to_remove != channel_mute.end());
        channel_mute.erase(channel_to_remove);

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

    std::unordered_map < int, bool > channel_mute;
};

#if 0
class DecibelSlider : public juce::Slider
{
public:
    DecibelSlider() = default;
    
    juce::String getTextFromValue(double value) override
    {
        return juce::Decibels::toString(slider_value_to_db((int)value));
    }
};

enum FaderStep {
    FaderStep_Editing,
    FaderStep_Hiding,
    FaderStep_Showing
};

class FaderComponent : public juce::Component
{
public:
    FaderComponent(double inital_gain,
                   FaderStep initial_step,
                   const juce::String &name,
                   std::function<void(double)> &&onFaderChange,
                   std::function<void(const juce::String&)> &&onEditedName)
    : onFaderChange(std::move(onFaderChange)),
        onEditedName(std::move(onEditedName))
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
        fader.setRange(0.0f, (double)(ArraySize(slider_values) - 1), 1.0);
        fader.setScrollWheelEnabled(false);
        
        fader.onValueChange = [this] {
            jassert(this->step == Editing);
            auto newGain = slider_value_to_gain((int)fader.getValue());
            this->onFaderChange(newGain);
        };
        addAndMakeVisible(fader);
        
        update(initial_step, initial_gain);
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
    
    void update(FaderStep new_step, double new_gain)
    {
        switch (new_step)
        {
            case FaderStep_Editing :
            {
                fader.setValue(gain_to_slider_value(new_gain), juce::dontSendNotification);
                fader.setEnabled(true);
                fader.setVisible(true);
            } break;
            case FaderStep_Hiding :
            {
                fader.setValue(gain_to_slider_value(new_gain), juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(false);
            } break;
            case FaderStep_Showing :
            {
                fader.setValue(gain_to_slider_value(new_gain), juce::dontSendNotification);
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
    std::function<void(double)> onFaderChange;
    std::function<void(const juce::String&)> onEditedName;
    FaderStep step;
    //double targetValue;
    //double smoothing;
};

class FaderRowComponent : public juce::Component
{
public:
    FaderRowComponent(std::unordered_map<int, std::unique_ptr<FaderComponent>>& faderComponents)
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DecibelSlider)
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
    
    void resizedChild(juce::Rectangle<int> bounds) override;
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