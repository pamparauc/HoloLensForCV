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

void VoiceRecognition::InitializeSpeechCommandList()
{
    m_lastCommand = nullptr;
    m_listening = false;
    m_speechCommandData = ref new Platform::Collections::Map<String^, float4>();

    m_speechCommandData->Insert(L"white", float4(1.0f, 1.0f, 1.0f, 1.f));
    m_speechCommandData->Insert(L"grey", float4(0.5f, 0.5f, 0.5f, 1.f));
    m_speechCommandData->Insert(L"green", float4(0.0f, 1.0f, 0.0f, 1.f));
    m_speechCommandData->Insert(L"black", float4(0.1f, 0.1f, 0.1f, 1.f));
    m_speechCommandData->Insert(L"red", float4(1.0f, 0.0f, 0.0f, 1.f));
    m_speechCommandData->Insert(L"yellow", float4(1.0f, 1.0f, 0.0f, 1.f));
    m_speechCommandData->Insert(L"aquamarine", float4(0.0f, 1.0f, 1.0f, 1.f));
    m_speechCommandData->Insert(L"blue", float4(0.0f, 0.0f, 1.0f, 1.f));
    m_speechCommandData->Insert(L"purple", float4(1.0f, 0.0f, 1.0f, 1.f));

    // You can use non-dictionary words as speech commands.
    m_speechCommandData->Insert(L"SpeechRecognizer", float4(0.5f, 0.1f, 1.f, 1.f));
}

Concurrency::task<void> VoiceRecognition::StopCurrentRecognizerIfExists()
{
    if (m_speechRecognizer != nullptr)
    {
        return create_task(m_speechRecognizer->StopRecognitionAsync()).then([this]()
            {
                m_speechRecognizer->RecognitionQualityDegrading -= m_speechRecognitionQualityDegradedToken;

                if (m_speechRecognizer->ContinuousRecognitionSession != nullptr)
                {
                    m_speechRecognizer->ContinuousRecognitionSession->ResultGenerated -= m_speechRecognizerResultEventToken;
                }
            });
    }
    else
    {
        return create_task([this]() {});
    }
}

bool VoiceRecognition::InitializeSpeechRecognizer()
{
    m_speechRecognizer = ref new SpeechRecognizer();

    if (!m_speechRecognizer)
    {
        return false;
    }

    m_speechRecognitionQualityDegradedToken = m_speechRecognizer->RecognitionQualityDegrading +=
        ref new TypedEventHandler<SpeechRecognizer^, SpeechRecognitionQualityDegradingEventArgs^>(
            std::bind(&VoiceRecognition::OnSpeechQualityDegraded, this, _1, _2)
            );

    m_speechRecognizerResultEventToken = m_speechRecognizer->ContinuousRecognitionSession->ResultGenerated +=
        ref new TypedEventHandler<SpeechContinuousRecognitionSession^, SpeechContinuousRecognitionResultGeneratedEventArgs^>(
            std::bind(&VoiceRecognition::OnResultGenerated, this, _1, _2)
            );

    return true;
}

task<bool> VoiceRecognition::StartRecognizeSpeechCommands()
{
    return StopCurrentRecognizerIfExists().then([this]()
        {
            if (!InitializeSpeechRecognizer())
            {
                return task_from_result<bool>(false);
            }

            // Here, we compile the list of voice commands by reading them from the map.
            Platform::Collections::Vector<String^>^ speechCommandList = ref new Platform::Collections::Vector<String^>();
            for each (auto pair in m_speechCommandData)
            {
                // The speech command string is what we are looking for here. Later, we can use the
                // recognition result for this string to look up a color value.
                auto command = pair->Key;

                // Add it to the list.
                speechCommandList->Append(command);
            }

            SpeechRecognitionListConstraint^ spConstraint = ref new SpeechRecognitionListConstraint(speechCommandList);
            m_speechRecognizer->Constraints->Clear();
            m_speechRecognizer->Constraints->Append(spConstraint);
            return create_task(m_speechRecognizer->CompileConstraintsAsync()).then([this](task<SpeechRecognitionCompilationResult^> previousTask)
                {
                    try
                    {
                        SpeechRecognitionCompilationResult^ compilationResult = previousTask.get();

                        if (compilationResult->Status == SpeechRecognitionResultStatus::Success)
                        {
                            // If compilation succeeds, we can start listening for results.
                            return create_task(m_speechRecognizer->ContinuousRecognitionSession->StartAsync()).then([this](task<void> startAsyncTask) {

                                try
                                {
                                    // StartAsync may throw an exception if your app doesn't have Microphone permissions. 
                                    // Make sure they're caught and handled appropriately (otherwise the app may silently not work as expected)
                                    startAsyncTask.get();
                                    return true;
                                }
                                catch (Exception^ exception)
                                {
                                    PrintWstringToDebugConsole(
                                        std::wstring(L"Exception while trying to start speech Recognition: ") +
                                        exception->Message->Data() +
                                        L"\n"
                                    );

                                    return false;
                                }
                                });
                        }
                        else
                        {
                            OutputDebugStringW(L"Could not initialize constraint-based speech engine!\n");

                            // Handle errors here.
                            return create_task([this] {return false; });
                        }
                    }
                    catch (Exception^ exception)
                    {
                        // Note that if you get an "Access is denied" exception, you might need to enable the microphone 
                        // privacy setting on the device and/or add the microphone capability to your app manifest.

                        PrintWstringToDebugConsole(
                            std::wstring(L"Exception while trying to initialize speech command list:") +
                            exception->Message->Data() +
                            L"\n"
                        );

                        // Handle exceptions here.
                        return create_task([this] {return false; });
                    }
                });
        });
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