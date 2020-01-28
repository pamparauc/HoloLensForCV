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
			faceCascade.load(cv::String("haarcascade_frontalface_alt.xml"));
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

		cv::Mat img_3C;
		cv::cvtColor(_resizedPVCameraImage, img_3C, CV_RGBA2BGR);

		// operations
		//redToBlue(_resizedPVCameraImage);

		//cv::Mat laplacian;
		//cv::Laplacian(_resizedPVCameraImage, laplacian, 0, 1);

		//Canny(_resizedPVCameraImage, _blurredPVCameraImage, _cannyPVCameraImage);

		//detectFaceOpenCVHaar(img_3C);
		cv::Mat out;
		/*out = modifyBrigthnessByValue(img_3C, -100);*/
		out = modifyContrastByValue(img_3C, 0.5);

		cv::Mat img_4C;
		cv::cvtColor(out, img_4C, CV_BGR2RGBA);


        OpenCVHelpers::CreateOrUpdateTexture2D(
            _deviceResources,
			img_4C,   // muchii peste red->blue
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

	void AppMain::detectFaceOpenCVHaar(cv::Mat& frameOpenCVHaar, int inHeight, int inWidth)
	{
		
		std::vector<Rect> faces;
	//	cvtColor(frameOpenCVHaar, frame_gray, CV_BGR2GRAY);
		//equalizeHist(frame_gray, frame_gray);
		//return 1;
		//-- Detect faces
		if (!faceCascade.empty())
		{
			faceCascade.detectMultiScale(frameOpenCVHaar, faces, 1.1, 3, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));

		}
		
		//for (unsigned int i = 0; i < faces.size(); ++i)
		//{
		//	cv::rectangle(frameOpenCVHaar, faces[i], cv::Scalar(0, 255, 0), 2);
		//}
		std::string text = std::to_string(faces.size()) + " faces";
		int fontFace = FONT_HERSHEY_SCRIPT_SIMPLEX;
		double fontScale = 1.5;
		int thickness = 3;
		int baseline = 0;
		Size textSize = getTextSize(text, fontFace,
			fontScale, thickness, &baseline);
		baseline += thickness;
		// center the text
		Point textOrg((frameOpenCVHaar.cols - textSize.width) / 2,
			(frameOpenCVHaar.rows + textSize.height) / 2);
		// draw the box
		rectangle(frameOpenCVHaar, textOrg + Point(0, baseline),
			textOrg + Point(textSize.width, -textSize.height),
			Scalar(0, 0, 255));
		// ... and the baseline first
		line(frameOpenCVHaar, textOrg + Point(0, thickness),
			textOrg + Point(textSize.width, thickness),
			Scalar(0, 0, 255));

		// then put the text itself
		putText(frameOpenCVHaar, text, textOrg, fontFace, fontScale,
			Scalar::all(255), thickness, 8);
	}

	std::string AppMain::get_http_data(const std::string& server, const std::string& file)
	{
		//CURL *curl;
		//FILE *fp;
		//CURLcode res;
		//char *url = "www.stud.usv.ro/~cpamparau/haarcascade_frontalface_alt.xml";
		//char outfilename[FILENAME_MAX] = "haarcascade_frontalface_alt.xml";
		//curl = curl_easy_init();
		//if (curl)
		//{
		//	fp = fopen(outfilename, "wb");
		//	curl_easy_setopt(curl, CURLOPT_URL, url);
		//	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		//	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		//	res = curl_easy_perform(curl);
		//	curl_easy_cleanup(curl);
		//	fclose(fp);
		//}
		return std::string("");
	}

	cv::Mat AppMain::modifyBrigthnessByValue(cv::Mat input, double value)
	{
		cv::Mat output;
		// value > 0 => increase brigthness
		// value < 0 => decrease brigthness
		input.convertTo(output, -1, 1, value);
		return output;
	}

	cv::Mat AppMain::modifyContrastByValue(cv::Mat input, double value)
	{
		cv::Mat output;
		// value < 1 => decrease contrast
		// value > 1 => increase contrast
		input.convertTo(output, -1, value, 0);
		return output;
	}
}
