#include "stdio.h"

class ProcessorHost;
class EditorHost;

class Application
{
public:
    Application(ProcessorHost &host);

    void initialiseEditorUI(EditorHost *new_editor);
    void onEditorDelete();
    void toMainMenu();
    void toGame(MixerGame_Variant variant);
    void toChannelSettings();
    void toStats();
    void toSettings();
    
    //TODO rename
    static std::unordered_map < uint32_t, Channel_DSP_State > bypassedAllChannelsDSP(const std::unordered_map<uint32_t, Game_Channel> &channels) {
        std::unordered_map < uint32_t, Channel_DSP_State > dsp_states;
        
        std::transform(channels.begin(), channels.end(), 
                       std::inserter(dsp_states, dsp_states.end()), 
                       [](const auto &a) -> std::pair<uint32_t, Channel_DSP_State>{
                       return { a.first, ChannelDSP_on() };
        });
        return dsp_states;
    }

    void broadcastDSP(const std::unordered_map < uint32_t, Channel_DSP_State > &dsp_states);

    void create_daw_channel(uint32_t id);
    void delete_daw_channel(uint32_t id);
    //void renameChannelFromUI(int id, juce::String newName);
    void rename_daw_channel(uint32_t id, const juce::String &new_name);
    void change_frequency_range_from_daw(uint32_t daw_channel_id, uint32_t game_channel_id, float new_min, float new_max);
    void bind_daw_channel_with_game_channel(uint32_t daw_track_id, uint32_t game_track_id);
    void set_model(const std::vector<Game_Channel> &channels);
    std::vector<Game_Channel> save_model();

private:
    enum PanelType{
        Panel_MainMenu,
        Panel_Game,
        Panel_Channel_Settings,
        Panel_Settings,
        Panel_Stats
    };
    
    std::vector<double> db_slider_values { -100.0, -12.0, -9.0, -6.0, -3.0 };
    std::unique_ptr<MixerGame_IO> game_io;
    MuliTrack_Model multitrack_model;
    MixerGameUI *game_ui;
    Settings settings = { 0.0f };
    Stats stats = { 0 };
    PanelType type;
    ProcessorHost &host;

    EditorHost *editor;
};