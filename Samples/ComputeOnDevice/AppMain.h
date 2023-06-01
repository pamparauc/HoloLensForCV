//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/photo/cuda.hpp>
#include <opencv2/photo.hpp>
#include <ppltasks.h>
#include <rapidjson/document.h>
#include <boost/asio.hpp>
#include "StepTimer.h"
#define RAPIDJSON_HAS_STDSTRING 1

namespace ComputeOnDevice
{
	class AppMain : public Holographic::AppMainBase
	{
	public:
		AppMain(
			const ::std::shared_ptr<Graphics::DeviceResources>& deviceResources);
		std::thread* tr = nullptr;
		std::thread* thread_audio = nullptr;
		// IDeviceNotify
		virtual void OnDeviceLost() override;
		void listening();
		virtual void OnDeviceRestored() override;

		// IAppMain
		virtual void OnHolographicSpaceChanged(
			_In_ Windows::Graphics::Holographic::HolographicSpace^ holographicSpace) override;

		virtual void OnUpdate(Windows::UI::Input::Spatial::SpatialPointerPose^ pointerPose,
			Windows::Foundation::Numerics::float3 const& offset,
			_In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
			_In_ const Graphics::StepTimer& stepTimer) override;

		virtual void OnPreRender() override;

		virtual void OnRender() override;

		virtual void InitDisplay() override;

		static cv::CascadeClassifier faceCascade;

		static Windows::Media::FaceAnalysis::FaceDetector^ m_FaceDetector;

		static bool wasLoaded ;

	private:
		static const unsigned int HResultPrivacyStatementDeclined = 0x80045509;
		// Initializes access to HoloLens sensors.
		void StartHoloLensMediaFrameSourceGroup();

		bool documentWasParsed = false;

		std::string data;
		rapidjson::Document document;
		void Listen();

		std::string get_http_data(const std::string& server, const std::string& file);

		Windows::UI::Core::CoreDispatcher^ dispatcher;
		Windows::Media::SpeechRecognition::SpeechRecognizer^ speechRecognizer;
		Windows::ApplicationModel::Resources::Core::ResourceContext^ speechContext;
		Windows::ApplicationModel::Resources::Core::ResourceMap^ speechResourceMap;


		//void permissions();
		void InitializeRecognizer();

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

		// Render loop timer.
		DX::StepTimer                                                   m_timer;

		// Initializes a speech recognizer.
		bool InitializeSpeechRecognizer();
		// Creates a speech command recognizer, and starts listening.
		Concurrency::task<bool> StartRecognizeSpeechCommands();

		// Resets the speech recognizer, if one exists.
		Concurrency::task<void> StopCurrentRecognizerIfExists();
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

		// Handles playback of speech synthesis audio.
		//OmnidirectionalSound                                            m_speechSynthesisSound;
		//OmnidirectionalSound                                            m_recognitionSound;
		//OmnidirectionalSound                                            m_startRecognitionSound;

		bool                                                            m_waitingForSpeechPrompt = false;
		bool                                                            m_waitingForSpeechCue = false;
		float                                                           m_secondsUntilSoundIsComplete = 0.f;

	private:
		std::vector<std::shared_ptr<Rendering::SlateRenderer> >_slateRendererList;
		std::shared_ptr<Rendering::SlateRenderer> _currentSlateRenderer;

		// Selected HoloLens media frame source group
		HoloLensForCV::MediaFrameSourceGroupType _selectedHoloLensMediaFrameSourceGroupType;
		HoloLensForCV::MediaFrameSourceGroup^ _holoLensMediaFrameSourceGroup;
		bool _holoLensMediaFrameSourceGroupStarted;

		// HoloLens media frame server manager
		HoloLensForCV::SensorFrameStreamer^ _sensorFrameStreamer;

		Windows::Foundation::DateTime _latestSelectedCameraTimestamp;

		cv::Mat _undistortMap1;
		cv::Mat _undistortMap2;
		bool _undistortMapsInitialized;

		cv::Mat _undistortedPVCameraImage;
		cv::Mat _resizedPVCameraImage;
		cv::Mat _blurredPVCameraImage;
		cv::Mat _cannyPVCameraImage;

		
		
		std::vector<Rendering::Texture2DPtr> _visualizationTextureList;
		Rendering::Texture2DPtr _currentVisualizationTexture;

		bool _isActiveRenderer;
		

		void changeColor(cv::Mat& Image, int oldR, int oldG, int oldB, int H);
		cv::Mat detectEdges(cv::Mat& original,  enum visualizeEdges type);

		cv::Mat detectFaces(cv::Mat input);

		cv::Mat adjustContrast(cv::Mat input, double value);

		cv::Mat adjustBrightness(cv::Mat input, double value);

		void applyVisualFilters(cv::Mat& videoFrame);

		void RGBtoHSV(int oldR, int oldG, int oldB, int& tolerance);

		cv::Mat& visualFilter(cv::Mat& videoFrame, int type, int number, ...);

		int getColorComponent(rapidjson::Value& val, std::string toFrom, std::string color);
	};


	enum visualFilter {
		ADJUST_CONTRAST = 0,
		ADJUST_BRIGHTNESS = 1,
		DETECT_FACES = 2,
		DETECT_EDGES = 3,
		CHANGE_COLOR = 4
	};

	enum visualizeEdges {
		HIGHLIGHT_EDGES = 0,
		HIGHLIGHT_BACKGROUND_OVER_EDGES = 1,
		COLOR_BACKGROUND_HIGHLIGHT_EDGES = 2,
		UNDEFINED = 3 // used for initializing 
	};
}
