/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
*/

#pragma once

namespace te = tracktion_engine;

//==============================================================================
/**
        Set of helper functions used in MainComponent.h
*/
//==============================================================================
namespace Helpers {
    
    // Adds and makes visible all the components in the second parameter to the component in the first parameter.
    static inline void addAndMakeVisible (Component& parent, const Array<Component*>& children) {
        for (auto c : children)
            parent.addAndMakeVisible (c);
    }

    // Returns an empty string if the first parameter is empty, returns the second paramter if not empty.
    static inline String getStringOrDefault (const String& stringToTest, const String& stringToReturnIfEmpty) {
        return stringToTest.isEmpty() ? stringToReturnIfEmpty : stringToTest;
    }

    // Called when the load button is clicked. Opens a file browser and lets the user pick a file.
    void browseForAudioFile (te::Engine& engine, std::function<void (const File&)> fileChosenCallback) {
        auto fc = std::make_shared<FileChooser> ("Please select an audio file to load...",
                                                 juce::File{},
                                                 engine.getAudioFileFormatManager().readFormatManager.getWildcardForAllFormats());

        fc->launchAsync (FileBrowserComponent::openMode + FileBrowserComponent::canSelectFiles,
                         [fc, &engine, callback = std::move (fileChosenCallback)] (const FileChooser&)
                         {
                             const auto f = fc->getResult();

                             if (f.existsAsFile())
                                 engine.getPropertyStorage().setDefaultLoadSaveDirectory ("apollon", f.getParentDirectory());

                             callback (f);
                         });
    }

    // Removes all the clips from the track.
    void removeAllClips (te::AudioTrack& track) {
        auto clips = track.getClips();

        for (int i = clips.size(); --i >= 0;)
            clips.getUnchecked (i)->removeFromParentTrack();
    }
    
    // Returns the i'th audio track into the given edit.
    te::AudioTrack* getOrInsertAudioTrackAt (te::Edit& edit, int i) {
        edit.ensureNumberOfAudioTracks (i + 1);
        return te::getAudioTracks (edit)[i];
    }

    // Loads an audio file into the given edit.
    te::WaveAudioClip::Ptr loadAudioFileAsClip (te::Edit& edit, const File& file) {
        // Find the first track and delete all clips from it.
        if (auto track = getOrInsertAudioTrackAt (edit, 0)) {
            

            // Add a new clip to this track.
            te::AudioFile audioFile (edit.engine, file);

            if (audioFile.isValid()){
                removeAllClips (*track);
                if (auto newClip = track->insertWaveClip (file.getFileNameWithoutExtension(), file,
                                                          { { 0.0, audioFile.getLength() }, 0.0 }, false)){
                    return newClip;
                    
                }
            }
        }

        return {};
    }

    // Configures the audio file to loop.
    template<typename ClipType>
    typename ClipType::Ptr loopAroundClip (ClipType& clip) {
        auto& transport = clip.edit.getTransport();
        transport.setLoopRange (clip.getEditTimeRange());
        transport.looping = true;
        transport.position = 0.0;
        transport.play (false);

        return clip;
    }

    // Plays or pauses the audio transport in the given edit.
    void togglePlay (te::Edit& edit) {
        auto& transport = edit.getTransport();

        // If its playing pause, if not play.
        transport.isPlaying() ? transport.stop (false, false) : transport.play (false);
    }
        


}
//==============================================================================
/**
        Thumbnail construct that displays the audio file and handles the cursor.
*/
//==============================================================================s
struct Thumbnail    : public Component {
    
    Thumbnail (te::TransportControl& tc)
        : transport (tc) {
        cursorUpdater.setCallback ([this]
                                   {
                                       updateCursorPosition();
                                       if (smartThumbnail.isGeneratingProxy() || smartThumbnail.isOutOfDate())
                                           repaint();
                                   });
        cursor.setFill (juce::Colours::orange);
        
        addAndMakeVisible (cursor);
    }

    void setFile (const te::AudioFile& file) {
        smartThumbnail.setNewFile (file);
        cursorUpdater.startTimerHz (25);
        cursor.setVisible(true);
        repaint();
    }

    void paint (Graphics& g) override {
        auto r = getLocalBounds();

        g.setColour(juce::Colours::darkgrey);
        g.fillRoundedRectangle(r.toFloat(), 10.0);
        
        if (smartThumbnail.isGeneratingProxy()) {
            g.setColour (juce::Colours::grey);
            g.drawText ("Loading File: " + String (roundToInt (smartThumbnail.getProxyProgress() * 100.0f)) + "%",
                        r, Justification::centred);

        }
        else {
            g.setColour (juce::Colours::white);
            smartThumbnail.drawChannels (g, r.reduced(0,10), true, { 0.0, smartThumbnail.getTotalLength() }, 1.0f);
        }
    }

    void mouseDown (const MouseEvent& e) override {
        transport.setUserDragging (true);
        mouseDrag (e);
    }

    void mouseDrag (const MouseEvent& e) override {
        jassert (getWidth() > 0);
        const float proportion = e.position.x / getWidth();
        transport.position = proportion * transport.getLoopRange().getLength();
    }

    void mouseUp (const MouseEvent&) override {
        transport.setUserDragging (false);
    }
    
    void clearFile () {
        smartThumbnail.file.deleteFile();
        cursor.setVisible(false);
    }

private:
    te::TransportControl& transport;
    te::SmartThumbnail smartThumbnail { transport.engine, te::AudioFile (transport.engine), *this, nullptr };
    DrawableRectangle cursor;
    te::LambdaTimer cursorUpdater;

    void updateCursorPosition(){
        const double loopLength = transport.getLoopRange().getLength();
        const double proportion = loopLength == 0.0 ? 0.0 : transport.getCurrentPosition() / loopLength;

        auto r = getLocalBounds().reduced(0,10).toFloat();
        const float x = r.getWidth() * float (proportion);
        cursor.setRectangle (r.withWidth (2.0f).withX (x));
    }
};
//==============================================================================
/**
    Wraps a te::AutomatableParameter as a juce::ValueSource so it can be used as
    a Value for example in a Slider.
*/
//==============================================================================
class ParameterValueSource  : public juce::Value::ValueSource,
                              private te::AutomatableParameter::Listener {
public:
                                  
    ParameterValueSource (te::AutomatableParameter::Ptr p)
        : param (p) {
        param->addListener (this);
    }
    
    ~ParameterValueSource() override {
        param->removeListener (this);
    }
    
    var getValue() const override {
        return param->getCurrentValue();
    }

    void setValue (const var& newValue) override {
        param->setParameter (static_cast<float> (newValue), juce::sendNotification);
    }

private:
    te::AutomatableParameter::Ptr param;
    
    void curveHasChanged (te::AutomatableParameter&) override {
        sendChangeMessage (false);
    }
    
    void currentValueChanged (te::AutomatableParameter&, float /*newValue*/) override {
        sendChangeMessage (false);
    }
};

/**
    Function that binds an AutomatableParameter to a Slider so changes in either are reflected across the other.
    Not in helper namespace since it needs to use the above ParamaterValueSource class.
 */
void bindSliderToParameter (juce::Slider& s, te::AutomatableParameter& p) {
    const auto v = p.valueRange;
    const auto range = NormalisableRange<double> (static_cast<double> (v.start),
                                                    static_cast<double> (v.end),
                                                    static_cast<double> (v.interval),
                                                    static_cast<double> (v.skew),
                                                    v.symmetricSkew);

    s.setNormalisableRange (range);
    s.getValueObject().referTo (juce::Value (new ParameterValueSource (p)));
}
//==============================================================================
/**
    Custom LookAndFeel class that dictates how to draw components on the screen.
*/
//==============================================================================
class ApollonLookAndFeel  : public juce::LookAndFeel_V4 {
public:
    
    // Function for determining the layout of a slider within its bounds
    Slider::SliderLayout getSliderLayout(Slider& s) override {
        Slider::SliderLayout layout;
        layout.sliderBounds = s.getBounds();
        
        return layout;
    };
    
    // Function to dictate the drawing of a slider
    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                                           const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override {
        
        auto outline = juce::Colours::darkgrey;
        auto fill    = juce::Colours::black;

        auto bounds = Rectangle<int> (x, y, width, height).toFloat();

        auto radius = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = jmin (12.0f, radius * 0.5f);

        Path backgroundArc;
        backgroundArc.addCentredArc (slider.getWidth()/2,
                                     bounds.getCentreY(),
                                     slider.getWidth()/2,
                                     slider.getHeight()/2,
                                     0.0f,
                                     rotaryStartAngle,
                                     rotaryEndAngle,
                                     true);

        g.setColour (outline);
        g.strokePath (backgroundArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));

        if (slider.isEnabled()) {
            Path valueArc;
            valueArc.addCentredArc (slider.getWidth()/2,
                                    bounds.getCentreY(),
                                    slider.getWidth()/2,
                                    slider.getHeight()/2,
                                    0.0f,
                                    2*3.1415,
                                    toAngle,
                                    true);
            
            g.setColour (fill);
            g.strokePath (valueArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::butt));
        }

        auto thumbWidth = lineW * 2.0f;
        Point<float> thumbPoint (slider.getWidth()/2 + (slider.getWidth()/2) * std::cos (toAngle - MathConstants<float>::halfPi),
                                 bounds.getCentreY() + (slider.getHeight()/2) * std::sin (toAngle - MathConstants<float>::halfPi));

        g.setColour (slider.findColour (Slider::thumbColourId));
        g.fillEllipse (Rectangle<float> (thumbWidth, thumbWidth).withCentre (thumbPoint));
         
        
    };
        
};
//==============================================================================
