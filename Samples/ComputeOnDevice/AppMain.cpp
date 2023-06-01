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
#include <algorithm>
#include "AudioCapturePermissions.h"

#include <windows.graphics.directx.direct3d11.interop.h>
#include <Collection.h>
using namespace cv;
using namespace Windows::Storage;
using namespace SDKTemplate;
using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::Media::SpeechRecognition;
using namespace Windows::ApplicationModel::Resources::Core;

//using namespace concurrency;
//using namespace Platform;
//using namespace Windows::Foundation;
//using namespace Windows::Foundation::Numerics;
//using namespace Windows::Graphics::Holographic;
//using namespace Windows::Media::SpeechRecognition;
//using namespace Windows::Perception::Spatial;
//using namespace Windows::UI::Input::Spatial;
//using namespace std::placeholders;


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
		//tr = new std::thread(&AppMain::listening, this);
		

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
		create_task(AudioCapturePermissions::RequestMicrophonePermissionAsync())
			.then([this](bool permissionGained)
				{
					if (permissionGained)
					{
						Windows::Globalization::Language^ speechLanguage = SpeechRecognizer::SystemSpeechLanguage;
						speechContext = ResourceContext::GetForCurrentView();
						//speechContext->Languages = ref new VectorView<String^>(1, speechLanguage->LanguageTag);

						speechResourceMap = ResourceManager::Current->MainResourceMap->GetSubtree(L"LocalizationSpeechResources");

						//PopulateLanguageDropdown();
						InitializeRecognizer();
					}
					else
					{
						InitializeRecognizer();
					}
				});

		//InitializeRecognizer();
		//thread_audio = new std::thread(&AppMain::Listen, this);
		//Listen();
    }

	void AppMain::listening()
	{
		WSADATA WSAData;

		SOCKET server, client;

		SOCKADDR_IN serverAddr, clientAddr;

		WSAStartup(MAKEWORD(2, 0), &WSAData);
		server = socket(AF_INET, SOCK_STREAM, 0);

		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(5555);
		bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		listen(server, 0);

		while (true)
		{
			char buffer[1024];
			int clientAddrSize = sizeof(clientAddr);
			if ((client = accept(server, (SOCKADDR*)&clientAddr, &clientAddrSize)) != INVALID_SOCKET)
			{
				recv(client, buffer, sizeof(buffer), 0);
				data = std::string(buffer, sizeof(buffer));
				if (!data.empty())
				{
					//document.Parse<rapidjson::kParseIterativeFlag, rapidjson::kParseFullPrecisionFlag>(data.c_str());
					document.ParseInsitu(const_cast<char*>(data.data()));
				}
				memset(buffer, 0, sizeof(buffer));

				closesocket(client);
			}
		}
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
		data = get_http_data("www.stud.usv.ro", "/~cpamparau/config.json");
		if (!data.empty())
		{
			//document.Parse<rapidjson::kParseIterativeFlag, rapidjson::kParseFullPrecisionFlag>(data.c_str());
			document.ParseInsitu(const_cast<char*>(data.data()));
		}
        UNREFERENCED_PARAMETER(holographicFrame);

        //dbg::TimerGuard timerGuard(
        //    L"AppMain::OnUpdate",
        //    30.0 /* minimum_time_elapsed_in_milliseconds */);

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
					//String w = "We set" + iter->Value.ToString() + "\n"
					dbg::TimerGuard timerGuard(
						std::wstring(L"We set") + command->Data() + L"\n",
						30.0 /* minimum_time_elapsed_in_milliseconds */);
					//PrintWstringToDebugConsole(std::wstring(L"We set") + command->Data() + L"\n");
					break;
				}
			}
		}


		m_timer.Tick([&]()
			{
				//
				// TODO: Update scene objects.
				//
				// Put time-based updates here. By default this code will run once per frame,
				// but if you change the StepTimer to use a fixed time step this code will
				// run as many times as needed to get to the current step.
				//

				//m_spinningCubeRenderer->Update(m_timer);

				// Wait to listen for speech input until the audible UI prompts are complete.
				//if ((m_waitingForSpeechPrompt == true) &&
				//    ((m_secondsUntilSoundIsComplete -= static_cast<float>(m_timer.GetElapsedSeconds())) <= 0.f))
				//{
				////    m_waitingForSpeechPrompt = false;
				//PlayRecognitionBeginSound();
				//}
				//else if ((m_waitingForSpeechCue == true) &&
				//    ((m_secondsUntilSoundIsComplete -= static_cast<float>(m_timer.GetElapsedSeconds())) <= 0.f))
				//{
				m_waitingForSpeechCue = false;
				m_secondsUntilSoundIsComplete = 0.f;
				StartRecognizeSpeechCommands();
				//}

			});
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
		tr->join();   
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

	cv::Mat AppMain::detectEdges(cv::Mat& original, enum ComputeOnDevice::visualizeEdges type)
	{
		cv::Mat canny;
		cv::Mat blurred;
		cv::medianBlur(
			original, // _resizedPVCameraImage
			blurred,// _blurredPVCameraImage
			3 /* ksize */);

		cv::Canny(
			blurred, // _blurredPVCameraImage
			canny, // _cannyPVCameraImage
			50.0,
			200.0);
		switch (type)
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
		return blurred;
	}

	cv::Mat AppMain::detectFaces(cv::Mat frameOpenCVHaar)
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
		return frameOpenCVHaar;
	}

	cv::Mat AppMain::adjustContrast(cv::Mat input, double value)
	{
		cv::Mat output;
		// value < 1 => decrease contrast
		// value > 1 => increase contrast
		input.convertTo(output, -1, value, 0);
		
		return output;
	}

	cv::Mat AppMain::adjustBrightness(cv::Mat input, double value)
	{
		cv::Mat output;
		// value > 0 => increase brigthness
		// value < 0 => decrease brigthness
		input.convertTo(output, -1, 1, value);
		return output;
	}

	void AppMain::applyVisualFilters(cv::Mat& videoFrame)
	{
		if (document.HasParseError())
		{
			rapidjson::ParseErrorCode error = document.GetParseError();
			return;
		}
		for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); 
			iter < document.MemberEnd(); ++iter)
		{
			std::string name = iter->name.GetString();
			if (name.find("Contrast") != std::string::npos) {
				std::string con = document[name.c_str()].GetString();
				double contrast = atof(con.c_str());
				visualFilter(videoFrame, ADJUST_CONTRAST, 1, contrast);
			}
			else if (name.find("Edge") != std::string::npos) {
				std::string type = document[name.c_str()].GetString();
				videoFrame = visualFilter(videoFrame.clone(), DETECT_EDGES, 1,  type);
			}
			else if (name.find("Color") != std::string::npos)
			{
				// (R,G,B) values of the color to be replaced
				rapidjson::Value& val = document[name.c_str()];
				int oldR = getColorComponent(val, "From", "R"),
					oldG = getColorComponent(val, "From", "G"),
					oldB = getColorComponent(val, "From", "B");
				// (R,G,B) values of the replacing color
				int newR = getColorComponent(val, "To", "R"),
					newG = getColorComponent(val, "To", "G"),
					newB = getColorComponent(val, "To", "B");
				// replace color based on  a similarity tolerance
				visualFilter(videoFrame, CHANGE_COLOR, 6,
					oldR, oldG, oldB, newR, newG, newB);
			}
			else if (name.find("Brightness") != std::string::npos) {
				std::string con = document[name.c_str()].GetString();
				con.erase(remove_if(con.begin(), con.end(), isspace), con.end());
				double brightness = atof(con.c_str());
				visualFilter(videoFrame, ADJUST_BRIGHTNESS, 1, brightness);
			}
			else if (name.find("detection") != std::string::npos ||
				name.find("Face") != std::string::npos)
				visualFilter(videoFrame, DETECT_FACES, 0);
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

	cv::Mat& AppMain::visualFilter(cv::Mat& videoFrame, int type, int number, ...)
	{
		cv::Mat& processedFrame = videoFrame.clone();
		va_list ap;
		va_start(ap, number);
		switch (type)
		{
			case visualFilter::ADJUST_CONTRAST:
			{
				double value =  va_arg(ap, double);
				va_end(ap);
				videoFrame = adjustContrast(processedFrame, value);
				break;
			}
			case visualFilter::ADJUST_BRIGHTNESS:
			{
				double value = va_arg(ap, double);
				va_end(ap);
				videoFrame = adjustBrightness(videoFrame, value);
				break;
			}
			case visualFilter::DETECT_EDGES:
			{
				std::string visualizeEdges = va_arg(ap, std::string);
				va_end(ap);
				ComputeOnDevice::visualizeEdges visualize = visualizeEdges::UNDEFINED;
				if (visualizeEdges == "Highlight Edges")
					visualize = visualizeEdges::HIGHLIGHT_EDGES;
				else if (visualizeEdges == "Highlight Background Over Edges")
					visualize = visualizeEdges::HIGHLIGHT_BACKGROUND_OVER_EDGES;
				else if (visualizeEdges == "Color Background Highlight Edges")
					visualize = visualizeEdges::COLOR_BACKGROUND_HIGHLIGHT_EDGES;
				videoFrame = detectEdges(videoFrame.clone(), visualize);
				break;
			}
			case visualFilter::DETECT_FACES:
			{
				detectFaces(videoFrame);
				break;
			}
			case visualFilter::CHANGE_COLOR:
			{
				int oldR = va_arg(ap, int);
				int oldG = va_arg(ap, int);
				int oldB = va_arg(ap, int);
				int newR = va_arg(ap, int);
				int newG = va_arg(ap, int);
				int newB = va_arg(ap, int);
				int	tolerance = 0;
				va_end(ap);
				RGBtoHSV(oldB, oldG, oldR, tolerance);
				changeColor(videoFrame, newR, newG, newB, tolerance);
				break;
			}
		}

		return videoFrame;
	}
	int AppMain::getColorComponent(rapidjson::Value& val, std::string toFrom, std::string color)
	{
		rapidjson::Value::ConstMemberIterator oldColorName = val.MemberBegin();
		std::string oldColor = oldColorName->name.GetString();
		if (toFrom == "From") 
			return document["Color-modification"]["From"][color.c_str()].GetInt();
		else if (toFrom == "To") 
			return document["Color-modification"]["To"][color.c_str()].GetInt();
	}

	void AppMain::InitializeRecognizer() {

			               if (this->speechRecognizer != nullptr)
			               {
			                     //speechRecognizer->StateChanged -= stateChangedToken;
			
			                     delete this->speechRecognizer;
			                     this->speechRecognizer = nullptr;
			               }
			
			               // Create an instance of SpeechRecognizer.
			               this->speechRecognizer = ref new SpeechRecognizer(SpeechRecognizer::SystemSpeechLanguage);
			
			               // Add a web search topic constraint to the recognizer.
			               auto webSearchConstraint = ref new SpeechRecognitionTopicConstraint(SpeechRecognitionScenario::WebSearch, "webSearch");
			               speechRecognizer->Constraints->Append(webSearchConstraint);
			
			
			               // Compile the constraint.
			               create_task(speechRecognizer->CompileConstraintsAsync())
			                     .then([this](task<SpeechRecognitionCompilationResult^> previousTask)
			                            {
			                                     //SpeechRecognitionCompilationResult^ compilationResult = previousTask.get();
			
			                                     //// Check to make sure that the constraints were in a proper format and the recognizer was able to compile it.
			                                     //if (compilationResult->Status != SpeechRecognitionResultStatus::Success)
			                                     //{
			
			                                     //}
			                             });
										
	}

	void AppMain::Listen(){
		
		try
		{
			// Start recognition.
			create_task(speechRecognizer->RecognizeWithUIAsync())
					.then([this](task<SpeechRecognitionResult^> recognitionTask)
						{
							try
							{
									SpeechRecognitionResult^ speechRecognitionResult = recognitionTask.get();
								// If successful, display the recognition result.
									if (speechRecognitionResult->Status == SpeechRecognitionResultStatus::Success)
									{
											//heardYouSayTextBlock->Visibility = Windows::UI::Xaml::Visibility::Visible;
											//resultTextBlock->Visibility = Windows::UI::Xaml::Visibility::Visible;
											//resultTextBlock->Text = speechRecognitionResult->Text;
											data = "{\"Users\":\"Primary User\",\"Brightness\":\"90\",\"Color - modification\":{\"From\":{\"R\":255,\"G\":0,\"B\":0},\"To\":{\"R\":0,\"G\":0,\"B\":255}}}";
									}
									else
									{
			
									}
							}
							catch (Platform::ObjectDisposedException^ exception)
							{
									// ObjectDisposedException will be thrown if you exit the scenario while the recogizer is actively
									// processing speech. Since this happens here when we navigate out of the scenario, don't try to
									// show a message dialog for this exception.
									OutputDebugString(L"ObjectDisposedException caught while recognition in progress (can be ignored):");
									OutputDebugString(exception->ToString()->Data());
							}
			
						});
		}
		catch (Platform::COMException^ exception)
		{
			OutputDebugString(L"ObjectDisposedException caught while recognition in progress (can be ignored):");
			OutputDebugString(exception->ToString()->Data());
		}
			
	}
	bool AppMain::InitializeSpeechRecognizer() {
		m_speechRecognizer = ref new SpeechRecognizer();

		if (!m_speechRecognizer)
		{
			return false;
		}

		m_speechRecognitionQualityDegradedToken = m_speechRecognizer->RecognitionQualityDegrading +=
			ref new TypedEventHandler<SpeechRecognizer^, SpeechRecognitionQualityDegradingEventArgs^>(
				std::bind(&AppMain::OnSpeechQualityDegraded, this)
				);

		m_speechRecognizerResultEventToken = m_speechRecognizer->ContinuousRecognitionSession->ResultGenerated +=
			ref new TypedEventHandler<SpeechContinuousRecognitionSession^, SpeechContinuousRecognitionResultGeneratedEventArgs^>(
				std::bind(&AppMain::OnResultGenerated, this)
				);

		return true;
	}


	// For speech example.
	// Change the cube color, if we get a valid result.
	void AppMain::OnResultGenerated(SpeechContinuousRecognitionSession^ sender, SpeechContinuousRecognitionResultGeneratedEventArgs^ args)
	{
		// For our list of commands, medium confidence is good enough. 
		// We also accept results that have high confidence.
		if ((args->Result->Confidence == SpeechRecognitionConfidence::High) ||
			(args->Result->Confidence == SpeechRecognitionConfidence::Medium))
		{
			m_lastCommand = args->Result->Text;

			// When the debugger is attached, we can print information to the debug console.
			dbg::TimerGuard timerGuard(
				std::wstring(L"Last command was: ") +
				m_lastCommand->Data() +
				L"\n",
				30.0 /* minimum_time_elapsed_in_milliseconds */);
			//PrintWstringToDebugConsole(
			//	
			//);

			// Play a sound to indicate a command was understood.
			//PlayRecognitionSound();
		}
		else
		{
			OutputDebugStringW(L"Recognition confidence not high enough.\n");
		}
	}

	void AppMain::OnSpeechQualityDegraded(Windows::Media::SpeechRecognition::SpeechRecognizer^ recognizer, Windows::Media::SpeechRecognition::SpeechRecognitionQualityDegradingEventArgs^ args)
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

	task<bool> AppMain::StartRecognizeSpeechCommands()
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
				auto constraint = ref new SpeechRecognitionTopicConstraint(SpeechRecognitionScenario::WebSearch, L"webSearch");
				m_speechRecognizer->Constraints->Clear();
				m_speechRecognizer->Constraints->Append(constraint);
				return create_task(m_speechRecognizer->CompileConstraintsAsync())
					.then([this](task<SpeechRecognitionCompilationResult^> previousTask)
						{
							try
							{
								SpeechRecognitionCompilationResult^ compilationResult = previousTask.get();

								// Check to make sure that the constraints were in a proper format and the recognizer was able to compile it.
								if (compilationResult->Status == SpeechRecognitionResultStatus::Success)
								{
									// If the compilation succeeded, we can start listening for the user's spoken phrase or sentence.
									create_task(m_speechRecognizer->RecognizeAsync()).then([this](task<SpeechRecognitionResult^>& previousTask)
										{
											try
											{
												auto result = previousTask.get();

												if (result->Status != SpeechRecognitionResultStatus::Success)
												{
													dbg::TimerGuard timerGuard(
														std::wstring(L"Speech recognition was not successful: ") +
														result->Status.ToString()->Data() +
														L"\n",
														30.0 /* minimum_time_elapsed_in_milliseconds */);
												}

												// In this example, we look for at least medium confidence in the speech result.
												if ((result->Confidence == SpeechRecognitionConfidence::High) ||
													(result->Confidence == SpeechRecognitionConfidence::Medium))
												{
													// If the user said a color name anywhere in their phrase, it will be recognized in the
													// Update loop; then, the cube will change color.
													m_lastCommand = result->Text;

													dbg::TimerGuard timerGuard(
														std::wstring(L"Speech phrase was: ") +
														result->Status.ToString()->Data() +
														L"\n",
														30.0 /* minimum_time_elapsed_in_milliseconds */);
												}
												else
												{
													dbg::TimerGuard timerGuard(
														std::wstring(L"Recognition confidence not high enough: ") +
														result->Status.ToString()->Data() +
														L"\n",
														30.0 /* minimum_time_elapsed_in_milliseconds */);
												}
											}
											catch (Exception exception)
											{
												// Note that if you get an "Access is denied" exception, you might need to enable the microphone
												// privacy setting on the device and/or add the microphone capability to your app manifest.
												//dbg::TimerGuard timerGuard(
												//	std::wstring(L"Speech recognizer error: ") +
												//	exception +
												//	L"\n",
												//	30.0 /* minimum_time_elapsed_in_milliseconds */);
											}
										});

									return true;
								}
								else
								{
									OutputDebugStringW(L"Could not initialize predefined grammar speech engine!\n");

									// Handle errors here.
									return false;
								}
							}
							catch (Exception exception)
							{
								// Note that if you get an "Access is denied" exception, you might need to enable the microphone
								//// privacy setting on the device and/or add the microphone capability to your app manifest.
								//dbg::TimerGuard timerGuard(
								//	std::wstring(L"Exception while trying to initialize predefined grammar speech engine: ") +
								//	exception->ToString()->Data() +
								//	L"\n",
								//	30.0 /* minimum_time_elapsed_in_milliseconds */);

								// Handle exceptions here.
								return false;
							}
						});
			});

	}

	Concurrency::task<void> AppMain::StopCurrentRecognizerIfExists()
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
}
