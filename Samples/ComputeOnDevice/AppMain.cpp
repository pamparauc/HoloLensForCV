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
#include <fstream>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <codecvt>

using namespace cv;
using namespace Windows::Storage;

using namespace Concurrency;

namespace ComputeOnDevice
{
	cv::CascadeClassifier AppMain::faceCascade;
	bool AppMain::wasLoaded = false;
    AppMain::AppMain(
        const std::shared_ptr<Graphics::DeviceResources>& deviceResources)
        : Holographic::AppMainBase(deviceResources)
        , _selectedHoloLensMediaFrameSourceGroupType(
            HoloLensForCV::MediaFrameSourceGroupType::PhotoVideoCamera)
		, _undistortMapsInitialized(false)
        , _holoLensMediaFrameSourceGroupStarted(false)
        , _isActiveRenderer(false)
    {
		data = get_http_data("www.stud.usv.ro", "/~cpamparau/config.json");
		if (!data.empty())
		{
			//document.Parse<rapidjson::kParseIterativeFlag, rapidjson::kParseFullPrecisionFlag>(data.c_str());
			document.ParseInsitu(const_cast<char*>(data.data()));
		}
		
		
		
		
		std::string filename = "haarcascade_frontalface_alt.xml"; //frontalface_alt
		
		//concurrency::task<StorageFile^> fileOperation =create_task(localFolder->CreateFileAsync("haarcascade_frontalface_alt.xml", CreationCollisionOption::ReplaceExisting));
		//fileOperation.then([this](StorageFile^ sampleFile)
		//{
		//	std::string outputXML = get_http_data("www.stud.usv.ro", "/~cpamparau/haarcascade_frontalface_alt.xml");

		//	return FileIO::WriteTextAsync(sampleFile, platformXML);
		//}).then([this](task<void> previousOperation) {
		//	try {
		//		previousOperation.get();
		//	}
		//	catch (Platform::Exception^) {
		//		// Timestamp not written
		//	}
		//});
		//Sleep(10000);
	    StorageFolder ^ localFolder = ApplicationData::Current->LocalFolder;
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::wstring intermediateForm = converter.from_bytes(filename);
		Platform::String^ platformXML = ref new Platform::String(intermediateForm.c_str());
		concurrency::task<StorageFile^> file =create_task( localFolder->GetFileAsync(platformXML));
		//while (file.is_done() == false)
		//{
		//	// wait to be done
		//}
		std::string numeFinal;
		file.then([this](StorageFile^ sampleFile)
		{
			Platform::String^ name = sampleFile->DisplayName;
			std::wstring fooW(name->Begin());
			std::string numeFinal(fooW.begin(), fooW.end());
			if (AppMain::faceCascade.load(cv::String(numeFinal.c_str())))
			{
				AppMain::wasLoaded = true;
			}
		});
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
		cv::Mat img_3C;
		cv::cvtColor(_resizedPVCameraImage, img_3C, CV_RGBA2BGR);
		int tip_3C = img_3C.type();

		applyVisualFilters(img_3C);

		cv::Mat img_4C;
		cv::cvtColor(img_3C, img_4C, CV_BGR2RGBA);


        OpenCVHelpers::CreateOrUpdateTexture2D(
            _deviceResources,
			img_4C,   
            _currentVisualizationTexture /*, CV_8UC1*/);
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

	void AppMain::changeColor(cv::Mat& Image, int newB, int newG, int newR, int H)
	{
		cv::Mat hsv;
		cv::cvtColor(Image, hsv, cv::COLOR_BGR2HSV);
		Mat mask1;
		// Creating masks to detect the upper and lower red color.
		// Opencv Uses other ranges for HSV Values https://www.informatalks.com/calculate-hsv-range-of-a-color-in-opencv/
		H /= 2; // S_opencv = 2.55*S_normal, V_openCV = 2.55*V_Normal
		inRange(hsv, Scalar(H>10?H-10:H, 100, 100), Scalar(H+10, 255, 255), mask1);
		Image.setTo(Scalar(newB, newG, newR), mask1); // to Blue
	}

	void AppMain::detectEdges(cv::Mat& original, cv::Mat& blurred, std::string visualizeEdges)
	{
		cv::Mat canny;
		cv::medianBlur(
			original, // _resizedPVCameraImage
			blurred,// _blurredPVCameraImage
			3 /* ksize */);

		cv::Canny(
			blurred, // _blurredPVCameraImage
			canny, // _cannyPVCameraImage
			50.0,
			200.0);
		int visualize = -1;
		if (visualizeEdges == "Highlight Edges")
			visualize = 0;
		else if (visualizeEdges == "Highlight Background Over Edges")
			visualize = 1;
		else if (visualizeEdges == "Color Background Highlight Edges")
			visualize = 2;
		switch (visualize)
		{
			case visualizeEdges::HIGHLIGHT_EDGES:
			{
				for (int32_t y = 0; y < blurred.rows; ++y)
				{
					for (int32_t x = 0; x < blurred.cols; ++x)
					{
						if (canny.at<uint8_t>(y, x) > 64)
						{
							*(blurred.ptr<uint32_t>(y, x)) = 0x00ff00; // green edges
						}
					}
				}
				break;
			}
			case visualizeEdges::HIGHLIGHT_BACKGROUND_OVER_EDGES:
			{
				for (int32_t y = 0; y < blurred.rows; ++y)
				{
					for (int32_t x = 0; x < blurred.cols; ++x)
					{
						if (canny.at<uint8_t>(y, x) > 64)
						{
							*(blurred.ptr<uint32_t>(y, x)) = 0x00ff00; // green edges
						}
						else
						{
							*(blurred.ptr<uint32_t>(y, x)) = 0x909090; // gray background
						}
					}
				}
				break;
			}
			case visualizeEdges::COLOR_BACKGROUND_HIGHLIGHT_EDGES:
			{
				for (int32_t y = 0; y < blurred.rows; ++y)
				{
					for (int32_t x = 0; x < blurred.cols; ++x)
					{
						if (canny.at<uint8_t>(y, x) > 64)
						{
							*(blurred.ptr<uint32_t>(y, x)) = 0x909090; // gray edges
						}
						else
						{
							*(blurred.ptr<uint32_t>(y, x)) = 0x00ff00; // green background  
						}
					}
				}
				break;
			}
		}
	}

	void AppMain::detectFaces(cv::Mat& frameOpenCVHaar)
	{
		std::vector<Rect> faces;
		Mat gray;
		cvtColor(frameOpenCVHaar, gray, COLOR_BGR2GRAY);
		if (wasLoaded)
		{
			//faceCascade.detectMultiScale(gray, faces, 3, 1);
			faceCascade.detectMultiScale(gray, faces, 3, 1); // for faces
			for (size_t i = 0; i < faces.size(); i++)
			{
				Rect r = faces[i]; 
				Scalar color = Scalar(255, 0, 0);
				rectangle(frameOpenCVHaar, cvPoint(r.x, r.y), cvPoint(r.x + r.width, r.y + r.height), color);
			}
		}
	
	}

	cv::Mat AppMain::adjustContrast(cv::Mat input, double value)
	{
		cv::Mat output;
		// value > 0 => increase brigthness
		// value < 0 => decrease brigthness
		input.convertTo(output, -1, 1, value);
		return output;
	}

	cv::Mat AppMain::adjustBrigthness(cv::Mat input, double value)
	{
		cv::Mat output;
		// value < 1 => decrease contrast
		// value > 1 => increase contrast
		input.convertTo(output, -1, value, 0);
		return output;
	}

	void AppMain::applyVisualFilters(cv::Mat& videoFrame)
	{
		if (document.HasParseError())
		{
			rapidjson::ParseErrorCode error = document.GetParseError();
			return;
		}
		for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter < document.MemberEnd(); ++iter)
		{
			std::string name = iter->name.GetString();
			if (name.find("Contrast") != std::string::npos) {
				double contrast = document[name.c_str()].GetDouble();
				videoFrame = adjustContrast(videoFrame.clone(), contrast);
			}
			else if (name.find("Edge") != std::string::npos)
				detectEdges(videoFrame.clone(), videoFrame, document[name.c_str()].GetString()); 
			else if (name.find("Color") != std::string::npos)
			{
				// values for the color that needs to be relaced
				rapidjson::Value::ConstMemberIterator oldColorName = document[name.c_str()].MemberBegin();
				std::string oldColor = oldColorName->name.GetString();
				int oldR = atoi(document[name.c_str()][oldColor.c_str()]["R"].GetString()),
					oldG = atoi(document[name.c_str()][oldColor.c_str()]["G"].GetString()),
					oldB = atoi(document[name.c_str()][oldColor.c_str()]["B"].GetString());

				//values for the color that will be replaced
				rapidjson::Value::ConstMemberIterator newColorName = ++oldColorName;
				std::string newColor = newColorName->name.GetString();
				int newR = atoi(document[name.c_str()][newColor.c_str()]["R"].GetString()), 
					newG = atoi(document[name.c_str()][newColor.c_str()]["G"].GetString()),
					newB = atoi(document[name.c_str()][newColor.c_str()]["B"].GetString());
				int tolerance = 0;
				RGBtoHSV(0, 255, 255, tolerance);
				changeColor(videoFrame, 65, 105, 225, tolerance);
			}
			else if (name.find("Brigthness") != std::string::npos) {
				double brigthness = document[name.c_str()].GetDouble();
				videoFrame = adjustBrigthness(videoFrame.clone(), brigthness);
			}
			else if (name.find("detection") != std::string::npos || name.find("Face") != std::string::npos)
				detectFaces(videoFrame);
		}
	}

	void AppMain::RGBtoHSV(int oldR, int oldG, int oldB, int& tolerance)
	{
		// according to https://www.rapidtables.com/convert/color/rgb-to-hsv.html
		// opencv works with BGR not RGB
		double Rp = oldR / 255, Gp = oldG / 255, Bp = oldB / 255;
		double CMax = std::max(Bp, std::max(Rp, Gp));
		double CMin = std::min(std::min(Rp, Gp), Bp);
		double delta = CMax - CMin;
		// determine Hue
		if (delta == 0)
		{
			tolerance = 0;
		}
		else if (delta == Rp)
		{
			double interm = (Gp - Bp) / delta;
			tolerance = 60 * ((int)interm % 6);
		}
		else if (delta == Gp)
		{
			double interm = (Bp - Rp) / delta;
			tolerance = 60 * (interm + 2);
		}
		else if (delta == Bp)
		{
			double interm = (Rp - Gp)/ delta;
			tolerance = 60 * (interm + 4);
		}
	}

	std::string AppMain::get_http_data(const std::string& server, const std::string& file)
	{
		using boost::asio::ip::tcp;
		boost::asio::io_service io_service;
		// Get a list of endpoints corresponding to the server name.
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(server, "http");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(io_service);
		boost::asio::connect(socket, endpoint_iterator);
		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << "GET " << file << " HTTP/1.0\r\n";
		request_stream << "Host: " << server << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n";

		// Send the request.
		boost::asio::write(socket, request);
		// Read the response status line. The response streambuf will automatically
		// grow to accommodate the entire line. The growth may be limited by passing
		// a maximum size to the streambuf constructor.
		boost::asio::streambuf response;
		boost::asio::read_until(socket, response, "\r\n");

		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/")
		{
			return "";
		}
		if (status_code != 200)
		{
			return "";
		}

		// Read the response headers, which are terminated by a blank line.
		boost::asio::read_until(socket, response, "\r\n\r\n");

		// Process the response headers.
		std::string header;
		while (std::getline(response_stream, header) && header != "\r")
			header += "\n";
		return { buffers_begin(response.data()), buffers_end(response.data()) };
	}

}
