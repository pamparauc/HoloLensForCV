#pragma once
#include "PrintWstringToDebugConsole.h"
class VoiceRecognition
{

public:
    VoiceRecognition();
    // Process continuous speech recognition results.
    void OnResultGenerated(
        Windows::Media::SpeechRecognition::SpeechContinuousRecognitionSession^ sender,
        Windows::Media::SpeechRecognition::SpeechContinuousRecognitionResultGeneratedEventArgs^ args
    );

    // Recognize when conditions might impact speech recognition quality.
    void OnSpeechQualityDegraded(
        Windows::Media::SpeechRecognition::SpeechRecognizer^ recognizer,
        Windows::Media::SpeechRecognition::SpeechRecognitionQualityDegradingEventArgs^ args
    );

    // Initializes the speech command list.
    void InitializeSpeechCommandList();

    // Initializes a speech recognizer.
    bool InitializeSpeechRecognizer();

    // Creates a speech command recognizer, and starts listening.
    Concurrency::task<bool> StartRecognizeSpeechCommands();

    // Resets the speech recognizer, if one exists.
    Concurrency::task<void> StopCurrentRecognizerIfExists();

    void UpdateListening(std::string* data);

private:
    // Event registration tokens.
    Windows::Foundation::EventRegistrationToken                     m_cameraAddedToken;
    Windows::Foundation::EventRegistrationToken                     m_cameraRemovedToken;
    Windows::Foundation::EventRegistrationToken                     m_locatabilityChangedToken;

    Windows::Foundation::EventRegistrationToken                     m_speechRecognizerResultEventToken;
    Windows::Foundation::EventRegistrationToken                     m_speechRecognitionQualityDegradedToken;

    bool                                                            m_listening;

    // Speech recognizer.
    Windows::Media::SpeechRecognition::SpeechRecognizer^ m_speechRecognizer;

    // Maps commands to color data.
    // We will create a Vector of the key values in this map for use as speech commands.
    Platform::Collections::Map<Platform::String^, Windows::Foundation::Numerics::float4>^ m_speechCommandData;

    // The most recent speech command received.
    Platform::String^ m_lastCommand;
    bool                                                            m_waitingForSpeechPrompt = false;
    bool                                                            m_waitingForSpeechCue = false;
    float                                                           m_secondsUntilSoundIsComplete = 0.f;
};
