
struct TextSlider : public juce::Slider
{
    TextSlider() : Slider(){}
    juce::String getTextFromValue(double pos) override { return get_text_from_value(pos); }
    std::function < juce::String(double) > get_text_from_value = [] (double){ return ""; };
};


class Continuous_Fader : public juce::Component
{
public:
    Continuous_Fader()
    {
        label.setText("Main Volume", juce::NotificationType::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setEditable(false);
        addAndMakeVisible(label);

        fader.get_text_from_value = [&] (double db) -> juce::String
        {
            if (db == -30.0)
                return "Muted";
            else
                return juce::Decibels::toString(db, 0);
        };

        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxBelow, true, fader.getTextBoxWidth(), 40);
        fader.setRange(-30.0, 0.0, 1);
        fader.setScrollWheelEnabled(true);

        fader.onValueChange = [this, &onMove = fader_moved_db_callback] {
            onMove(fader.getValue());
        };
        addAndMakeVisible(fader);
    }


    void resized() override
    {
        auto r = getLocalBounds();
        auto labelBounds = r.removeFromTop(50);
        label.setBounds(labelBounds);

        auto faderBounds = r;
        fader.setBounds(faderBounds);
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(r.toFloat(), 5.0f, 2.0f);
    }

    void update(double new_value_db)
    {
        fader.setValue(new_value_db);
    }
    std::function < void(double) > fader_moved_db_callback;

private:
    juce::Label label;
    TextSlider fader;
};

class FaderComponent : public juce::Component
{
public:
    explicit FaderComponent(const std::vector<double> &dbValues,
                            const juce::String &name,
                            std::function<void(int)> &&onFaderMove)
    : db_values(dbValues)
    {
        label.setText(name, juce::NotificationType::dontSendNotification);
        label.setEditable(false);
        addAndMakeVisible(label);
        
        fader.get_text_from_value = [&] (double pos) -> juce::String
        {
            double db = db_values[static_cast<size_t>(pos)];
            if (db == -100.0) 
                return "Muted";
            else
                return juce::Decibels::toString(db, 0);
        };
        
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxBelow, true, fader.getTextBoxWidth(), 40);
        fader.setRange(0.0, (double)(db_values.size() - 1), 1.0);
        fader.setScrollWheelEnabled(true);
        
        fader.onValueChange = [this, onMove = std::move(onFaderMove)] {
            assert(this->DEBUG_step == Widget_Editing);
            onMove((int)fader.getValue());
        };
        addAndMakeVisible(fader);
    }
    
    
    void resized() override
    {
        auto r = getLocalBounds();
        auto labelBounds = r.removeFromTop(50);
        label.setBounds(labelBounds);
        
        auto faderBounds = r;
        fader.setBounds(faderBounds);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(r.toFloat(), 5.0f, 2.0f);
    }
    
    void setTrackName(const juce::String& new_name)
    {
        label.setText(new_name, juce::dontSendNotification);
    }
    
    void update(Widget_Interaction_Type new_step, int new_pos)
    {
        DEBUG_step = new_step;
        switch (new_step)
        {
            case Widget_Editing :
            {
                fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(true);
                fader.setVisible(true);
            } break;
            case Widget_Hiding :
            {
                assert(new_pos == -1);
                //TODO ?
                //fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(false);
            } break;
            case Widget_Showing :
            {
                fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(true);
            } break;
        }

        repaint();
    }
    
private:
    Widget_Interaction_Type DEBUG_step;
    juce::Label label;
    TextSlider fader;
    const std::vector<double> &db_values;
    //double targetValue;
    //double smoothing;
};

class FaderRowComponent : public juce::Component
{
public:
    FaderRowComponent(std::vector<std::unique_ptr<FaderComponent>>& mixerFaders)
    : faders(mixerFaders)
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
        for (auto& fader : faders)
        {
            fader->setTopLeftPosition(top_left + juce::Point<int>(x_offset, 0));
            fader->setSize(fader_width, bounds.getHeight());
            x_offset += fader_width;
        }
        adjustWidth();
    }
    
private:
    const static int fader_width = 60;
    std::vector<std::unique_ptr<FaderComponent>> &faders;
};


struct Frequency_Bounds_Widget : juce::Component
{
public:
    Frequency_Bounds_Widget()
    {
        
        minLabel.setJustificationType(juce::Justification::right);
        addAndMakeVisible(minLabel);
        
        maxLabel.setJustificationType(juce::Justification::left);
        addAndMakeVisible(maxLabel);
        
        frequencyRangeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        frequencyRangeSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
        frequencyRangeSlider.setRange(0.0f, 1.0f);

        auto update_labels = [this] (int min_freq, int max_freq)
        {
            auto min_text = juce::String(min_freq) + " Hz";
            auto max_text = juce::String(max_freq) + " Hz";
            minLabel.setText(min_text, juce::dontSendNotification);
            maxLabel.setText(max_text, juce::dontSendNotification);
        };

        frequencyRangeSlider.onValueChange = [this, update_labels]{
            float minValue = (float)frequencyRangeSlider.getMinValue();
            float maxValue = (float)frequencyRangeSlider.getMaxValue();
            float new_min_frequency = denormalize_slider_frequency(minValue);
            float new_max_frequency = denormalize_slider_frequency(maxValue);
            update_labels((int)new_min_frequency, (int) new_max_frequency);
            if(on_mix_max_changed)
                on_mix_max_changed(new_min_frequency, new_max_frequency, false);
        };

        frequencyRangeSlider.onDragEnd = [this, update_labels]{
            float minValue = (float)frequencyRangeSlider.getMinValue();
            float maxValue = (float)frequencyRangeSlider.getMaxValue();
            float new_min_frequency = denormalize_slider_frequency(minValue);
            float new_max_frequency = denormalize_slider_frequency(maxValue);
            update_labels((int)new_min_frequency, (int) new_max_frequency);
            if(on_mix_max_changed)
                on_mix_max_changed(new_min_frequency, new_max_frequency, true);
        };
        addAndMakeVisible(frequencyRangeSlider);
    }

    void setMinAndMaxValues(float min_frequency, float max_frequency)
    {
        frequencyRangeSlider.setMinAndMaxValues(
            normalize_slider_frequency(min_frequency), 
            normalize_slider_frequency(max_frequency)
        );
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto min_label_bounds = bounds.removeFromLeft(70);
        auto max_label_bounds = bounds.removeFromRight(70);

        minLabel.setBounds(min_label_bounds);
        frequencyRangeSlider.setBounds(bounds);
        maxLabel.setBounds(max_label_bounds);
    }
    std::function < void(float, float, bool) > on_mix_max_changed;

private:
    juce::Label minLabel;
    juce::Label maxLabel;
    juce::Slider frequencyRangeSlider;
};

class List_Row_Label  : public juce::Label
{
public:
    List_Row_Label (
        std::string insertionRowText,
        const std::function < void(int) > &onMouseDown,
        const std::function < void(int, std::string) > &onTextChanged,
        const std::function < void(std::string) > &onCreateRow
    )
    : mouse_down_callback(onMouseDown),
      text_changed_callback(onTextChanged),
      create_row_callback(onCreateRow),
      insertion_row_text(insertionRowText)
    {
        // double click to edit the label text; single click handled below
        setEditable (false, true, true);
    }

    void mouseDown (const juce::MouseEvent& ) override
    {   
        mouse_down_callback(row);
        //juce::Label::mouseDown (event);
    }

    void editorShown (juce::TextEditor * new_editor) override
    {
        if(is_last_row)
            new_editor->setText("", juce::dontSendNotification);
    }

    void textWasEdited() override
    {
        bool input_is_empty = getText().isEmpty();

        //rename track
        if (!is_last_row)
        {
            if (input_is_empty)
            {
                setText(row_text, juce::dontSendNotification);
                return;
            }
            text_changed_callback(row, getText().toStdString());
        }
        //insert new track
        else
        {
                
            if (input_is_empty)
            {
                setText(insertion_row_text, juce::dontSendNotification);
                return;
            }
            is_last_row = false;
            create_row_callback(getText().toStdString());
        }
    }

    void update(int new_row, std::string new_text, bool new_is_last_row)
    {
        row = new_row;
        row_text = new_text;
        is_last_row = new_is_last_row;
        if (!is_last_row)
        {
            setJustificationType(juce::Justification::left);
            setColour(juce::Label::textColourId, juce::Colours::white);
            setText (row_text, juce::dontSendNotification);
        }
        else
        {
            setJustificationType(juce::Justification::centred);
            setColour(juce::Label::textColourId, juce::Colours::lightgrey);
            setText (insertion_row_text, juce::dontSendNotification);
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto& lf = getLookAndFeel();
        if (! dynamic_cast<juce::LookAndFeel_V4*>(&lf))
            lf.setColour (textColourId, juce::Colours::black);
    
        juce::Label::paint (g);
    }

    const std::function < void(int) > &mouse_down_callback;
    const std::function < void(int, std::string) > &text_changed_callback;
    const std::function < void(std::string) > &create_row_callback;

    int row;
    std::string row_text;
    bool is_last_row;
private:
    std::string insertion_row_text;
    juce::Colour textColour;
};


class Insertable_List : public juce::Component,
public juce::ListBoxModel
{
public:
    Insertable_List(std::string insertionRowText)
    : insertion_row_text { insertionRowText }
    {
        list_comp.setModel(this);
        list_comp.setMultipleSelectionEnabled(false);
        addAndMakeVisible(list_comp);

        editable_mouse_down_callback = [&] (int row_idx) {
            list_comp.selectRow(row_idx);
        };
        editable_text_changed_callback = [&] (int row_idx, std::string row_text) {
            assert(row_idx <= checked_cast<int>(rows_text.size()));
            assert(row_idx >= 0);
            rename_channel_callback(row_idx, row_text);
        };
        editable_create_row_callback = [&] (std::string new_row_text) {
            create_channel_callback(new_row_text);
        };
    }

    virtual ~Insertable_List() override = default;

    void resized() override
    {
        list_comp.setBounds(getLocalBounds());
    }

    int getNumRows() override { return static_cast<int>(rows_text.size()) + 1; }

    void paintListBoxItem (int rowNumber,
                           juce::Graphics& g,
                           int width, int height,
                           bool row_is_selected) override
    {
        auto alternateColour = getLookAndFeel().findColour (juce::ListBox::backgroundColourId)
            .interpolatedWith (getLookAndFeel().findColour (juce::ListBox::textColourId), 0.03f);
        if (rowNumber % 2)
            g.fillAll (alternateColour);

        if (row_is_selected && rowNumber < getNumRows())
        {
            g.setColour(juce::Colours::white);
            auto bounds = juce::Rectangle { 0, 0, width, height };
            g.drawRect(bounds);
        }
    }

    void returnKeyPressed(int) override
    {
        auto selected_row = list_comp.getSelectedRow();
        if(selected_row == -1)
            return;
        auto *row_component = list_comp.getComponentForRowNumber(selected_row);
        auto *label = dynamic_cast<List_Row_Label*>(row_component);
        assert(label);
        label->showEditor();
    }

    void deleteKeyPressed (int) override
    {
        auto row_to_delete = list_comp.getSelectedRow();
        if(row_to_delete == -1 || row_to_delete == getNumRows() - 1) 
            return;

        int row_to_select{};
        if (row_to_delete == 0)
            row_to_select = 0;
        else if (row_to_delete == getNumRows() - 2)
            row_to_select = row_to_delete - 1;
        else 
            row_to_select = row_to_delete;

        delete_channel_callback(row_to_delete);
        list_comp.selectRow(row_to_select);
    }

    bool keyPressed (const juce::KeyPress &key) override
    {
        if (key == key.escapeKey)
        {
            list_comp.deselectAllRows();
            return true;
        }
        return false;
    }

    
    juce::Component *refreshComponentForRow (int row_number,
                                             bool,
                                             juce::Component *existing_component) override
    {
        int num_rows = checked_cast<int>(rows_text.size());
        if (row_number > num_rows)
        {
            if (existing_component != nullptr)
                delete existing_component;
            return nullptr;
        }
        else
        {
            List_Row_Label *label;

            if (existing_component != nullptr)
            {
                label = dynamic_cast<List_Row_Label*>(existing_component);
                assert(label != nullptr);
            }
            else
            {
                label = new List_Row_Label(insertion_row_text,
                                           editable_mouse_down_callback,
                                           editable_text_changed_callback,
                                           editable_create_row_callback);
            }
            std::string row_text = row_number < num_rows
                ? rows_text[row_number] 
                : "";
            label->update(row_number, row_text, row_number == num_rows);
            if(row_number < num_rows)
                customization_point(row_number, label);
            return label;
        } 
    }
    
    
    void selectedRowsChanged (int last_row_selected) override
    {   
        if(last_row_selected != -1 && 
            checked_cast<size_t>(last_row_selected) != rows_text.size())
            selected_channel_changed_callback(last_row_selected);
    }

    void update(std::vector<std::string> new_rows_text)
    {
        rows_text = new_rows_text;
        list_comp.updateContent();
    }

    void select_row(int row_idx)
    {
        list_comp.selectRow(row_idx);
    }


    std::function<void(int)> selected_channel_changed_callback = {};
    std::function<void(std::string)> create_channel_callback = {};
    std::function<void(int)> delete_channel_callback = {};
    std::function<void(int, std::string)> rename_channel_callback = {};
    std::function<void(int, List_Row_Label*)> customization_point = {};

    juce::ListBox list_comp;
    std::string insertion_row_text;
private:
    std::function<void(int)> editable_mouse_down_callback;
    std::function<void(int, std::string)> editable_text_changed_callback;
    std::function<void(std::string)> editable_create_row_callback;

    std::vector<std::string> rows_text;
};

class Selection_List : 
    public juce::Component,
public juce::ListBoxModel
{
public:
    explicit Selection_List(bool multiple_selection_enabled)
    {
        list_comp.setMultipleSelectionEnabled(multiple_selection_enabled);
        list_comp.setClickingTogglesRowSelection(true);
        addAndMakeVisible(list_comp);
    }

    int getNumRows() override
    {
        return checked_cast<int>(row_texts.size());
    }

    void paintListBoxItem (int rowNumber,
                           juce::Graphics &g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        
        auto alternateColour = getLookAndFeel().findColour (juce::ListBox::backgroundColourId)
            .interpolatedWith (getLookAndFeel().findColour (juce::ListBox::textColourId), 0.03f);
        if (rowNumber % 2)
            g.fillAll (alternateColour);
        
        if (rowNumber >= getNumRows()) 
            return;
        auto bounds = juce::Rectangle { 0, 0, width, height };
        if (rowIsSelected)
            g.setColour(juce::Colours::white);
        else
            g.setColour(juce::Colours::grey);
        g.drawText(row_texts[rowNumber], bounds.withTrimmedLeft(5), juce::Justification::centredLeft);
    }

    void selectedRowsChanged(int) override
    {
        auto juce_selection = list_comp.getSelectedRows();
        std::vector<bool> std_selection(getNumRows(), false);;
        for (auto i = 0; i < juce_selection.size(); i++)
        {
            int selected_idx = juce_selection[i];
            std_selection[selected_idx] = true;
        }
        //TODO slow ??? who cares ?
        selection_changed_callback(&std_selection);
    }

    
    bool keyPressed (const juce::KeyPress &key) override
    {
        if (key == key.escapeKey)
        {
            list_comp.deselectAllRows();
            return true;
        }
        return false;
    }

    void resized() override
    {
        list_comp.setBounds(getLocalBounds());
    }

    void set_rows(const std::vector<juce::String> &rowTexts,
                  const std::vector<bool> &selection)
    {
        assert(selection.size() == rowTexts.size());
        row_texts = std::move(rowTexts);

        juce::SparseSet < int > selected_juce{};
        for (uint32_t i = 0; i < selection.size(); i++)
        {
            if(selection[i])
                selected_juce.addRange(juce::Range<int>(i, i + 1));
        }
        list_comp.updateContent();
        list_comp.setSelectedRows (selected_juce, juce::dontSendNotification);
    }

    
    void select_row(int new_row)
    {
        list_comp.selectRow(new_row);
    }

    std::function < void(std::vector<bool> *) > selection_changed_callback;

private:
    std::vector<juce::String> row_texts;
    juce::ListBox list_comp = { {}, this };;
    std::function<void(int)> row_on_mouse_down;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Selection_List)
};


class Add_Delete_Move_Column : public juce::Component
{
public:
    Add_Delete_Move_Column()
    {
    }

        void add_button(juce::Path path, std::function<void()> && on_click = []{})
    {        
        auto button_colour = findColour (juce::ListBox::textColourId);
        auto new_button = std::make_unique<juce::DrawableButton>( juce::String{}, juce::DrawableButton::ButtonStyle::ImageOnButtonBackground);
        juce::DrawablePath image {};
        image.setFill (button_colour);
        image.setPath(path);
        new_button->setImages(&image);
        new_button->onClick = [on_click = std::move(on_click)] {
            on_click();
        };
        addAndMakeVisible(*new_button);
        buttons.emplace_back(std::move(new_button));
    }

    void paint(juce::Graphics &g) override
    {
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto width = getWidth();
        auto rect = juce::Rectangle<int> { width, width };
        for (auto i = 0; i < buttons.size(); i++)
        {
            buttons[i]->setBounds(rect.translated(0, width * i));

        }

    }

private:
    std::vector<std::unique_ptr<juce::DrawableButton>> buttons;
};