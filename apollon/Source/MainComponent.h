#pragma once

#include <JuceHeader.h>
#include "Utilities.h"

using namespace tracktion_engine;


//==============================================================================
/**
    Main Component class which lives inside our window, and this is where you should put all
    your controls and content.
*/
//==============================================================================

class MainComponent  : public juce::Component,
                       private ChangeListener {
public:
                           
    // MainComponent Constructor.
    MainComponent() {
        // Apply custom LookAndFeel class to the project.
        juce::LookAndFeel::setDefaultLookAndFeel(&lnf);
        
        
        // Sets the initial size of the screen and grabs focus.
        setSize (300, 300);
        setWantsKeyboardFocus(true);
        
        // Registers this ChangeListener with the audio transport.
        transport.addChangeListener(this);
        
        
        // Adds all elements to the MainComponent and makes them visible.
        Helpers::addAndMakeVisible(*this,
                                   {&playPauseButton, &loadFileButton, &thumbnail, &pitchShiftSlider});
        
        // Sets behavior of buttons when pressed.
        playPauseButton.onClick = [this] {if(loaded) Helpers::togglePlay(edit);}; // Plays the file if it was loaded
        loadFileButton.onClick = [this] {Helpers::browseForAudioFile(engine, [this] (const File& f) {f.exists() ? setFile (f) : noFileChosen(); });}; // Loads the chosen file if it exists
        
        // Makes sure clicking the buttons doesn't change their state (i.e. changing the button image).
        playPauseButton.setClickingTogglesState(false);
        loadFileButton.setClickingTogglesState(false);
        
        // Sets the images of the buttons.
        updatePlayButtonText();
        loadFileButton.setImages(false, true, false, load_white, 1.0f, {}, load_black, 1.0f, {}, load_white, 1.0f, {});
        

        // Setup pitch shifting.
        {
            // Register the PitchShiftPlugin with the engine.
            engine.getPluginManager().createBuiltInType<PitchShiftPlugin>();
            
            // Create new instance of plugin and insert in track 1.
            auto pitchShiftPlugin = edit.getPluginCache().createNewPlugin(PitchShiftPlugin::xmlTypeName, {});
            
            auto track = Helpers::getOrInsertAudioTrackAt(edit, 0);
            track->pluginList.insertPlugin(pitchShiftPlugin, 0, nullptr);
            
            // Connect slider value.
            auto pitchShiftParam = pitchShiftPlugin->getAutomatableParameterByID ("semitones up");
            bindSliderToParameter(pitchShiftSlider, *pitchShiftParam);
            pitchShiftSlider.setSkewFactorFromMidPoint(0.0);
            
            
            // Remove text box from the slider.
            pitchShiftSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, 0, 0, 0);
            
            // Extra slider alterations to make it arc and reset on double click.
            pitchShiftSlider.setColour(juce::Slider::ColourIds::thumbColourId, juce::Colours::orange);
            pitchShiftSlider.setSliderStyle(juce::Slider::RotaryHorizontalDrag);
            pitchShiftSlider.setRotaryParameters(3*(3.1415/2) + (3.1415/4), 5*(3.1415/2) - (3.1415/4), true);
            pitchShiftSlider.setRange(-4.0, 4.0); // Slider can transpose from -7 to 7 semitones
            pitchShiftSlider.setValue(0.0);
            pitchShiftSlider.setDoubleClickReturnValue(true, 0.0, {});
            
        }
        
    }

    // MainComponent Destructor.
    ~MainComponent() override {
        edit.getTempDirectory(false).deleteRecursively();
    }

    //==============================================================================
    
    // Sets the bounds of gui elements.
    void paint (juce::Graphics& g) override {
        g.fillAll(juce::Colours::grey); // Paint background grey
        
        int x_offset = screen_width/12;
        int y_offset = screen_height/12;
        
        pitchShiftSlider.setBounds(x_offset, y_offset, screen_width - (2*x_offset), 2*y_offset);
        
        thumbnail.setBounds(x_offset, 4*y_offset, screen_width - (2*x_offset), 4*y_offset);
        
        loadFileButton.setBounds(x_offset, 9*y_offset, x_offset+y_offset, x_offset+y_offset); // Width and height are the average of the offsets (i.e. 2 * ((x_offset + y_offset) / 2) simplified)
        playPauseButton.setBounds(9*x_offset, 9*y_offset, x_offset+y_offset, x_offset+y_offset);
        
    }

    // Reset the screen width and height on resize.
    void resized() override {
        screen_width = getWidth();
        screen_height = getHeight();
    }

    // Handles keyboard input.
    bool keyPressed(const KeyPress &k) override {
        // Trigger play/pause button when spacebar is hit
        if(k.getKeyCode() == k.spaceKey){
            playPauseButton.onClick();
        }
        return false;
    }

private:
    // PRIVATE MEMBER VARIABLES
    //==============================================================================
    
    // Establish screen width and height.
    int screen_width = getWidth();
    int screen_height = getHeight();
    
    // Custom LookAndFeel Class.
    ApollonLookAndFeel lnf;
    
    // Tracktion engine objects.
    te::Engine engine {ProjectInfo::projectName};
    te::Edit edit {engine, te::createEmptyEdit (engine), te::Edit::forEditing, nullptr, 0};
    te::TransportControl& transport {edit.getTransport()};

    // File chooser for loading audio files.
    FileChooser audioFileChooser { "Load an audio file...",
                                   engine.getPropertyStorage().getDefaultLoadSaveDirectory ("apollon"),
                                   engine.getAudioFileFormatManager().readFormatManager.getWildcardForAllFormats() };

    // GUI elements.
    ImageButton playPauseButton, loadFileButton;
    Thumbnail thumbnail {transport};
    Slider pitchShiftSlider;
    
    // Booloean that keeps track of wether an audio track was loaded into the transport.
    bool loaded = false;
    
    // Create the images to display for the play/pause and load file buttons.
    Image load_white = ImageCache::getFromMemory(BinaryData::load_white_png, BinaryData::load_white_pngSize);
    Image load_black = ImageCache::getFromMemory(BinaryData::load_black_png, BinaryData::load_black_pngSize);
    
    Image play_white = ImageCache::getFromMemory(BinaryData::play_white_png, BinaryData::play_white_pngSize);
    Image play_black = ImageCache::getFromMemory(BinaryData::play_black_png, BinaryData::play_black_pngSize);
    
    Image pause_white = ImageCache::getFromMemory(BinaryData::pause_white_png, BinaryData::pause_white_pngSize);
    Image pause_black = ImageCache::getFromMemory(BinaryData::pause_black_png, BinaryData::pause_black_pngSize);
    
    
    //==============================================================================
    
    // PRIVATE MEMBER FUNCTIONS
    //==============================================================================
    
    // Sets the file in the transport, if possible.
    void setFile(const File& f) {
        if(auto clip = Helpers::loadAudioFileAsClip(edit, f)) {
            clip->setAutoTempo(false);
            clip->setAutoPitch(false);
            clip->setTimeStretchMode(te::TimeStretcher::melodyne);
            
            thumbnail.setFile(Helpers::loopAroundClip (*clip)->getPlaybackFile());
        }
        else {
            thumbnail.setFile({engine});
        }
        
        // If the file exists then it was loaded, update loaded boolean accordingly.
        f.exists() ? loaded = true : loaded = false;
        transport.stop(false, false);
    }
    
    // Sets the play/pause image button to its normal state, and updates its images based on wether audio is being played.
    void updatePlayButtonText() {
        playPauseButton.setState(Button::ButtonState::buttonNormal); // The state of the play/pause button never changes, just the button images change
        
        if(transport.isPlaying())
            playPauseButton.setImages(false, true, false, pause_white, 1.0f, {}, pause_black, 1.0f, {}, pause_white, 1.0f, {});
        else
            playPauseButton.setImages(false, true, false, play_white, 1.0f, {}, play_black, 1.0f, {}, play_white, 1.0f, {});
        
    }
    
    // Called when the user does not chose a valid file after clicking the load file button.
    void noFileChosen() {
        thumbnail.clearFile();
        loaded = false;
    }
    
    void changeListenerCallback(ChangeBroadcaster*) override {
        updatePlayButtonText();
    }
    //==============================================================================


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
