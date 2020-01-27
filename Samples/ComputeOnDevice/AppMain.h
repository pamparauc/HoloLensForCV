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
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
//#pragma comment(lib, "crypt32")
//#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "wldap32.lib")
//#define CURL_STATICLIB
//#include "curl/curl.h"

namespace ComputeOnDevice
{
    class AppMain : public Holographic::AppMainBase
    {
    public:
        AppMain(
            const std::shared_ptr<Graphics::DeviceResources>& deviceResources);

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
		
    private:
        // Initializes access to HoloLens sensors.
        void StartHoloLensMediaFrameSourceGroup();
		
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

		cv::CascadeClassifier faceCascade;

        std::vector<Rendering::Texture2DPtr> _visualizationTextureList;
        Rendering::Texture2DPtr _currentVisualizationTexture;

        bool _isActiveRenderer;

		void redToBlue(cv::Mat& Image);
		void Canny(cv::Mat& original, cv::Mat& blurred, cv::Mat& canny);

		void FaceDetection(cv::Mat& input);

		int detectFaceOpenCVHaar(cv::Mat frameOpenCVHaar, int inHeight = 300, int inWidth = 0);
	
		std::string get_http_data(const std::string& server, const std::string& file);
    };
}
