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
#include "VoiceRecognition.h"
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

		static cv::CascadeClassifier faceCascade;

		static Windows::Media::FaceAnalysis::FaceDetector^ m_FaceDetector;

		static bool wasLoaded;

	private:
		// Initializes access to HoloLens sensors.
		void StartHoloLensMediaFrameSourceGroup();

		bool documentWasParsed = false;

		std::string data;
		rapidjson::Document document;
		void Listen();

		std::string get_http_data(const std::string& server, const std::string& file);

		static const unsigned int HResultPrivacyStatementDeclined = 0x80045509;

		Windows::UI::Core::CoreDispatcher^ dispatcher;
		Windows::Media::SpeechRecognition::SpeechRecognizer^ speechRecognizer;
		Windows::ApplicationModel::Resources::Core::ResourceContext^ speechContext;
		Windows::ApplicationModel::Resources::Core::ResourceMap^ speechResourceMap;

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

		VoiceRecognition* voice = nullptr;

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
		cv::Mat detectEdges(cv::Mat& original, enum visualizeEdges type);

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