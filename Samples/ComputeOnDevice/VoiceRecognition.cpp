#include "pch.h"
#include "VoiceRecognition.h"

using namespace HolographicVoiceInput;
using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Media::SpeechRecognition;
using namespace Windows::Perception::Spatial;
using namespace Windows::UI::Input::Spatial;
using namespace std::placeholders;

VoiceRecognition::VoiceRecognition() {

}

void VoiceRecognition::UpdateListening(std::string& data) {
    // Check for new speech input since the last frame.
    if (m_lastCommand != nullptr)
    {
        auto command = m_lastCommand;
        m_lastCommand = nullptr;

        // Check to see if the spoken word or phrase, matches up with any of the speech
        // commands in our speech command map.
        for each (auto & iter in m_speechCommandData)
        {
            std::wstring lastCommandString = command->Data();
            std::wstring listCommandString = iter->Key->Data();

            if (lastCommandString.find(listCommandString) != std::wstring::npos)
            {
                // If so, we can set the cube to the color that was spoken.
                //m_spinningCubeRenderer->SetColor(iter->Value);
                data = "";
                break;
            }
        }
    }
}
// For speech example.
// Change the cube color, if we get a valid result.
void VoiceRecognition::OnResultGenerated(SpeechContinuousRecognitionSession^ sender, SpeechContinuousRecognitionResultGeneratedEventArgs^ args)
{
    // For our list of commands, medium confidence is good enough. 
    // We also accept results that have high confidence.
    if ((args->Result->Confidence == SpeechRecognitionConfidence::High) ||
        (args->Result->Confidence == SpeechRecognitionConfidence::Medium))
    {
        m_lastCommand = args->Result->Text;

        // When the debugger is attached, we can print information to the debug console.
        PrintWstringToDebugConsole(
            std::wstring(L"Last command was: ") +
            m_lastCommand->Data() +
            L"\n"
        );

        // Play a sound to indicate a command was understood.
        //PlayRecognitionSound();
    }
    else
    {
        OutputDebugStringW(L"Recognition confidence not high enough.\n");
    }
}

void VoiceRecognition::OnSpeechQualityDegraded(Windows::Media::SpeechRecognition::SpeechRecognizer^ recognizer, Windows::Media::SpeechRecognition::SpeechRecognitionQualityDegradingEventArgs^ args)
{
    switch (args->Problem)
    {
    case SpeechRecognitionAudioProblem::TooFast:
        OutputDebugStringW(L"The user spoke too quickly.\n");
        break;

    case SpeechRecognitionAudioProblem::TooSlow:
        OutputDebugStringW(L"The user spoke too slowly.\n");
        break;

    case SpeechRecognitionAudioProblem::TooQuiet:
        OutputDebugStringW(L"The user spoke too softly.\n");
        break;

    case SpeechRecognitionAudioProblem::TooLoud:
        OutputDebugStringW(L"The user spoke too loudly.\n");
        break;

    case SpeechRecognitionAudioProblem::TooNoisy:
        OutputDebugStringW(L"There is too much noise in the signal.\n");
        break;

    case SpeechRecognitionAudioProblem::NoSignal:
        OutputDebugStringW(L"There is no signal.\n");
        break;

    case SpeechRecognitionAudioProblem::None:
    default:
        OutputDebugStringW(L"An error was reported with no information.\n");
        break;
    }
}