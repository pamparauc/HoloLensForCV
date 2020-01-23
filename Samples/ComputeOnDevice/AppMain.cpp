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

#include "pch.h"

#include "AppMain.h"
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
namespace ComputeOnDevice
{
    AppMain::AppMain(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : Holographic::AppMainBase(deviceResources)
        , _selectedHoloLensMediaFrameSourceGroupType(
            HoloLensForCV::MediaFrameSourceGroupType::PhotoVideoCamera)
        , _holoLensMediaFrameSourceGroupStarted(false)
        , _undistortMapsInitialized(false)
        , _isActiveRenderer(false)
    {
    }

    void AppMain::OnHolographicSpaceChanged(
        Windows::Graphics::Holographic::HolographicSpace^ holographicSpace)
    {
        //
        // Initialize the HoloLens media frame readers
        //
        StartHoloLensMediaFrameSourceGroup();
    }
	void AppMain::InitDisplay()
	{
		if (!_isActiveRenderer)
		{
			_currentSlateRenderer =
				std::make_shared<Rendering::SlateRenderer>(
					_deviceResources);
			_slateRendererList.push_back(_currentSlateRenderer);
			_isActiveRenderer = true;
		}
	}

    void AppMain::OnUpdate(Windows::UI::Input::Spatial::SpatialPointerPose^ pointerPose,
		Windows::Foundation::Numerics::float3 const& offset,
        _In_ Windows::Graphics::Holographic::HolographicFrame^ holographicFrame,
        _In_ const Graphics::StepTimer& stepTimer)
    {

        UNREFERENCED_PARAMETER(holographicFrame);

        dbg::TimerGuard timerGuard(
            L"AppMain::OnUpdate",
            30.0 /* minimum_time_elapsed_in_milliseconds */);

        //
        // Update scene objects.
        //
        // Put time-based updates here. By default this code will run once per frame,
        // but if you change the StepTimer to use a fixed time step this code will
        // run as many times as needed to get to the current step.
        //
        
        for (auto& r : _slateRendererList)
        {
            r->Update(pointerPose, offset,
                stepTimer);
        }
        
        
        //
        // Process sensor data received through the HoloLensForCV component.
        //
        if (!_holoLensMediaFrameSourceGroupStarted)
        {
            return;
        }

        HoloLensForCV::SensorFrame^ latestFrame;

        latestFrame =
            _holoLensMediaFrameSourceGroup->GetLatestSensorFrame(
                HoloLensForCV::SensorType::PhotoVideo);

        if (nullptr == latestFrame)
        {
            return;
        }

        if (_latestSelectedCameraTimestamp.UniversalTime == latestFrame->Timestamp.UniversalTime)
        {
            return;
        }

        _latestSelectedCameraTimestamp = latestFrame->Timestamp;

        cv::Mat wrappedImage;

        rmcv::WrapHoloLensSensorFrameWithCvMat(
            latestFrame,
            wrappedImage);

        if (!_undistortMapsInitialized)
        {
            Windows::Media::Devices::Core::CameraIntrinsics^ cameraIntrinsics =
                latestFrame->CoreCameraIntrinsics;

            if (nullptr != cameraIntrinsics)
            {
                cv::Mat cameraMatrix(3, 3, CV_64FC1);

                cv::setIdentity(cameraMatrix);

                cameraMatrix.at<double>(0, 0) = cameraIntrinsics->FocalLength.x;
                cameraMatrix.at<double>(1, 1) = cameraIntrinsics->FocalLength.y;
                cameraMatrix.at<double>(0, 2) = cameraIntrinsics->PrincipalPoint.x;
                cameraMatrix.at<double>(1, 2) = cameraIntrinsics->PrincipalPoint.y;

                cv::Mat distCoeffs(5, 1, CV_64FC1);

                distCoeffs.at<double>(0, 0) = cameraIntrinsics->RadialDistortion.x;
                distCoeffs.at<double>(1, 0) = cameraIntrinsics->RadialDistortion.y;
                distCoeffs.at<double>(2, 0) = cameraIntrinsics->TangentialDistortion.x;
                distCoeffs.at<double>(3, 0) = cameraIntrinsics->TangentialDistortion.y;
                distCoeffs.at<double>(4, 0) = cameraIntrinsics->RadialDistortion.z;

                cv::initUndistortRectifyMap(
                    cameraMatrix,
                    distCoeffs,
                    cv::Mat_<double>::eye(3, 3) /* R */,
                    cameraMatrix,
                    cv::Size(wrappedImage.cols, wrappedImage.rows),
                    CV_32FC1 /* type */,
                    _undistortMap1,
                    _undistortMap2);

                _undistortMapsInitialized = true;
            }
        }

        if (_undistortMapsInitialized)
        {
            cv::remap(
                wrappedImage,
                _undistortedPVCameraImage,
                _undistortMap1,
                _undistortMap2,
                cv::INTER_LINEAR);

            cv::resize(
                _undistortedPVCameraImage,
                _resizedPVCameraImage,
                cv::Size(),
                0.5 /* fx */,
                0.5 /* fy */,
                cv::INTER_AREA);
        }
        else
        {
            cv::resize(
                wrappedImage,
                _resizedPVCameraImage,
                cv::Size(),
                0.5 /* fx */,
                0.5 /* fy */,
                cv::INTER_AREA);
        }

		
		//redToBlue(_resizedPVCameraImage);
		
		//cv::Mat laplacian;
		//cv::Laplacian(_resizedPVCameraImage, laplacian, 0, 1);
		
		//Canny(_resizedPVCameraImage, _blurredPVCameraImage, _cannyPVCameraImage);
		
		
		//CascadeClassifier faceCascade;
		//detectFaceOpenCVHaar(faceCascade, _resizedPVCameraImage);

        OpenCVHelpers::CreateOrUpdateTexture2D(
            _deviceResources,
			_resizedPVCameraImage,   // muchii peste red->blue
            _currentVisualizationTexture);
    }

    void AppMain::OnPreRender()
    {
    }

    // Renders the current frame to each holographic camera, according to the
    // current application and spatial positioning state.
    void AppMain::OnRender()
    {
        // Draw the sample hologram.
        //for (size_t i = 0; i < _visualizationTextureList.size(); ++i)
        //{
        //    _slateRendererList[i]->Render(
        //        _visualizationTextureList[i]);
        //}
        //
        if (_isActiveRenderer)
        {
            _currentSlateRenderer->Render(_currentVisualizationTexture);
        }
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // need to be released before this method returns.
    void AppMain::OnDeviceLost()
    {
        
        for (auto& r : _slateRendererList)
        {
            r->ReleaseDeviceDependentResources();
        }

        _holoLensMediaFrameSourceGroup = nullptr;
        _holoLensMediaFrameSourceGroupStarted = false;

        for (auto& v : _visualizationTextureList)
        {
            v.reset();
        }
        _currentVisualizationTexture.reset();
    }

    // Notifies classes that use Direct3D device resources that the device resources
    // may now be recreated.
    void AppMain::OnDeviceRestored()
    {
        for (auto& r : _slateRendererList)
        {
            r->CreateDeviceDependentResources();
        }

        StartHoloLensMediaFrameSourceGroup();
    }

    void AppMain::StartHoloLensMediaFrameSourceGroup()
    {
        _sensorFrameStreamer =
            ref new HoloLensForCV::SensorFrameStreamer();

        _sensorFrameStreamer->EnableAll();

        _holoLensMediaFrameSourceGroup =
            ref new HoloLensForCV::MediaFrameSourceGroup(
                _selectedHoloLensMediaFrameSourceGroupType,
                _spatialPerception,
                _sensorFrameStreamer);

        _holoLensMediaFrameSourceGroup->Enable(
            HoloLensForCV::SensorType::PhotoVideo);

        concurrency::create_task(_holoLensMediaFrameSourceGroup->StartAsync()).then(
            [&]()
        {
            _holoLensMediaFrameSourceGroupStarted = true;
        });
    }

	// imageProcessing

	void AppMain::redToBlue(cv::Mat& Image)
	{
		cv::Mat hsv;
		cv::cvtColor(Image, hsv, cv::COLOR_RGB2HSV);
		Mat mask1;
		// Creating masks to detect the upper and lower blue color.
		inRange(hsv, Scalar(110, 50, 50), Scalar(130, 255, 255), mask1);
		Image.setTo(Scalar(255, 0, 0), mask1);
	}

	void AppMain::Canny(cv::Mat& original, cv::Mat& blurred, cv::Mat& canny)
	{
		cv::medianBlur(
			original, // _resizedPVCameraImage
			blurred,// _blurredPVCameraImage
			3 /* ksize */);

		cv::Canny(
			blurred, // _blurredPVCameraImage
			canny, // _cannyPVCameraImage
			50.0,
			200.0);

		for (int32_t y = 0; y < blurred.rows; ++y)
		{
			for (int32_t x = 0; x < blurred.cols; ++x)
			{
				if (canny.at<uint8_t>(y, x) > 64)
				{
					*(blurred.ptr<uint32_t>(y, x)) = 0xCD0000;
				}
			}
		}
	}

	void AppMain::detectFaceOpenCVHaar(cv::CascadeClassifier faceCascade, cv::Mat &frameOpenCVHaar, int inHeight, int inWidth)
	{
		int frameHeight = frameOpenCVHaar.rows;
		int frameWidth = frameOpenCVHaar.cols;
		if (!inWidth)
			inWidth = (int)((frameWidth / (float)frameHeight) * inHeight);

		float scaleHeight = frameHeight / (float)inHeight;
		float scaleWidth = frameWidth / (float)inWidth;

		Mat frameOpenCVHaarSmall, frameGray;
		resize(frameOpenCVHaar, frameOpenCVHaarSmall, Size(inWidth, inHeight));
		cvtColor(frameOpenCVHaarSmall, frameGray, COLOR_BGR2GRAY);

		std::vector<Rect> faces;
		faceCascade.detectMultiScale(frameOpenCVHaar, faces);

		for (size_t i = 0; i < faces.size(); i++)
		{
			int x1 = (int)(faces[i].x * scaleWidth);
			int y1 = (int)(faces[i].y * scaleHeight);
			int x2 = (int)((faces[i].x + faces[i].width) * scaleWidth);
			int y2 = (int)((faces[i].y + faces[i].height) * scaleHeight);
			rectangle(frameOpenCVHaar, Point(x1, y1), Point(x2, y2), Scalar(0, 255, 0), (int)(frameHeight / 150.0), 4);
		}
	}
}
