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

#define RAPIDJSON_HAS_STDSTRING 1

namespace ComputeOnDevice
{
	class AppMain : public Holographic::AppMainBase
	{
	public:
		AppMain(
			const ::std::shared_ptr<Graphics::DeviceResources>& deviceResources);

		// IDeviceNotify
		virtual void OnDeviceLost() override;

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
		// Initializes access to HoloLens sensors.
		void StartHoloLensMediaFrameSourceGroup();

		bool documentWasParsed = false;

		std::string data;
		rapidjson::Document document;


		std::string get_http_data(const std::string& server, const std::string& file);

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
		

		void changeColor(cv::Mat& Image, int oldR, int oldG, int oldB, int H, int S, int V);
		void Canny(cv::Mat& original, cv::Mat& blurred);

		void FaceDetection(cv::Mat& input);

		cv::Mat modifyBrigthnessByValue(cv::Mat input, double value);

		cv::Mat modifyContrastByValue(cv::Mat input, double value);

		cv::Mat grabCut(cv::Mat input);

		cv::Mat backrgoundSubstraction(cv::Mat input);

		void performImageProcessingAlgorithms(cv::Mat& inputOutput);

		void determineHSVvaluesForRGBColor(int oldR, int oldG, int oldB, int& newR, int& newG, int& newB );
	};
}
