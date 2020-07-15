#include "napi.h"
#include "obs.h"
#include "RerunOBSClient.h"
#include "RerunOBSScene.h"
#include <iostream>
#include <thread>
#include "Windows.h"

RerunOBSClient::RerunOBSClient(const Napi::CallbackInfo &info) : Napi::ObjectWrap<RerunOBSClient>(info) {}

//Launch the OBS thread with the specified settings and initialize the video and audio systems
Napi::Value RerunOBSClient::init(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env(); //The stack context of this method call
	if (info.Length() != 3)
	{
		throw Napi::Error::New(env, "Invalid number of arguments");
	}

	if (!info[0].IsObject() || !info[1].IsObject() || !info[2].IsString())
	{
		throw Napi::Error::New(env, "Invalid arguments");
	}

	Napi::Object videoSettings = info[0].As<Napi::Object>();
	std::string graphicsModule = videoSettings.Get("module").As<Napi::String>();
	uint32_t fps = videoSettings.Get("fps").As<Napi::Number>().Uint32Value();
	uint32_t screenWidth = videoSettings.Get("width").As<Napi::Number>().Uint32Value();
	uint32_t screenHeight = videoSettings.Get("height").As<Napi::Number>().Uint32Value();

	Napi::Object audioSettings = info[1].As<Napi::Object>();
	uint32_t audioSamples = audioSettings.Get("samples").As<Napi::Number>().Uint32Value();
	speaker_layout speakerLayout = static_cast<speaker_layout>(audioSettings.Get("speakerLayout").As<Napi::Number>().Int32Value());

	std::string moduleConfigpath = info[2].As<Napi::String>();

	//Start OBS subprocess
	bool contextLaunched = obs_startup("en-US", moduleConfigpath.c_str(), NULL);
	if (!contextLaunched)
	{
		throw Napi::Error::New(env, "Failed to start OBS subprocess");
	}

	//Initialize video subsystem
	obs_video_info vInfo = {
		graphicsModule.c_str(),	   //Graphics_module - can be opengl or d3d11. d3d11 recommended for windows
		fps, 1U,				   //Output fps numerator and denominator (30 frames per second)
		screenWidth, screenHeight, //Base compositing width, height
		screenWidth, screenHeight, //Output width, height
		VIDEO_FORMAT_RGBA		   //Video format (pixel format)
	};
	int videoStatus = obs_reset_video(&vInfo);

	if (videoStatus != OBS_VIDEO_SUCCESS)
	{
		obs_shutdown();
		if (videoStatus == OBS_VIDEO_CURRENTLY_ACTIVE)
		{
			throw Napi::Error::New(env, "Error initializing video subsystem - the video subsystem is currently in use");
		}
		else if (videoStatus == OBS_VIDEO_NOT_SUPPORTED)
		{
			throw Napi::Error::New(env, "Error initializing video subsystem - graphics adapter not supported");
		}
		else if (videoStatus == OBS_VIDEO_MODULE_NOT_FOUND)
		{
			throw Napi::Error::New(env, "Error initializing video subsystem - Direct3D 11 module not found");
		}
		else
		{
			throw Napi::Error::New(env, "Error initializing video subsystem - generic failure");
		}
	}

	//Initialize audio subsystem
	obs_audio_info aInfo = {
		audioSamples,  //Audio samples/sec
		speakerLayout, //Speaker layout
	};
	if (!obs_reset_audio(&aInfo))
	{
		obs_shutdown();
		throw Napi::Error::New(env, "Error initializing audio subsystem");
	}

	//Rerun only uses one scene in OBS. Create it now
	Napi::Object mainScene = RerunOBSScene::constructor.New({Napi::String::New(env, "MainScene"), Napi::Number::New(env, 0)}).As<Napi::Object>();
	mainSceneObj = Napi::ObjectReference::New(mainScene, 1); //Maintains a reference to the scene object wrap so it isn't garbage collected by Node

	return Napi::Boolean::New(env, true);
}

//Open and initialize a module at the provided path
void RerunOBSClient::loadModule(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env(); //The stack context of this method call
	if (info.Length() != 2)
	{
		throw Napi::Error::New(env, "Invalid number of arguments");
	}

	if (!info[0].IsString() || !info[1].IsString())
	{
		throw Napi::Error::New(env, "Invalid arguments");
	}

	std::string moduleBinaryPath = info[0].As<Napi::String>();
	std::string moduleDataPath = info[1].As<Napi::String>();

	//Some modules (lookin' at you CHROME) seem to die if these paths use back slashes
	std::replace(moduleBinaryPath.begin(), moduleBinaryPath.end(), '\\', '/');
	std::replace(moduleDataPath.begin(), moduleDataPath.end(), '\\', '/');

	obs_module_t *module;
	int openStatus = obs_open_module(&module, moduleBinaryPath.c_str(), moduleDataPath.c_str());

	if (openStatus != MODULE_SUCCESS)
	{
		if (openStatus == MODULE_FILE_NOT_FOUND)
		{
			throw Napi::Error::New(env, "file not found");
		}
		else if (openStatus == MODULE_MISSING_EXPORTS)
		{
			throw Napi::Error::New(env, "missing required exports");
		}
		else if (openStatus == MODULE_INCOMPATIBLE_VER)
		{
			throw Napi::Error::New(env, "incompatible OBS version");
		}
		else
		{
			throw Napi::Error::New(env, "generic failure");
		}
	}

	if (!obs_init_module(module))
	{
		throw Napi::Error::New(env, "initialization failed");
	}
}

//Setup the audio and video encoders
void RerunOBSClient::setupEncoders(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env(); //The stack context of this method call
	if (info.Length() != 2)
	{
		throw Napi::Error::New(env, "Invalid number of arguments");
	}

	if (!info[0].IsObject() || !info[1].IsObject())
	{
		throw Napi::Error::New(env, "Invalid arguments");
	}

	Napi::Object videoConfig = info[0].As<Napi::Object>();
	std::string vEncodeType = videoConfig.Get("encoder").As<Napi::String>();
	Napi::Object vEncodeConfig = videoConfig.Get("encoderSettings").As<Napi::Object>();

	Napi::Object audioConfig = info[1].As<Napi::Object>();
	std::string aEncodeType = audioConfig.Get("encoder").As<Napi::String>();
	Napi::Object aEncodeConfig = audioConfig.Get("encoderSettings").As<Napi::Object>();

	//Configure video encoder
	obs_data_t *vEncoderSettings = createDataFromJS(vEncodeConfig);
	RerunOBSClient::vEncode = obs_video_encoder_create(vEncodeType.c_str(), "vEncoder", vEncoderSettings, NULL);

	if (RerunOBSClient::vEncode == NULL)
	{
		throw Napi::Error::New(env, "Error configuring video encoder - failed to create " + vEncodeType + " video encoder");
	}

	obs_encoder_set_video(RerunOBSClient::vEncode, obs_get_video()); //Pipe OBS sources video frames -> vEncode

	//Configure audio encoder
	obs_data_t *aEncodeSettings = createDataFromJS(aEncodeConfig);
	RerunOBSClient::aEncode = obs_audio_encoder_create(aEncodeType.c_str(), "aEncoder", aEncodeSettings, 0, NULL);

	if (RerunOBSClient::aEncode == NULL)
	{
		throw Napi::Error::New(env, "Error configuring audio encoder - failed to create " + aEncodeType + " audio encoder");
	}

	obs_encoder_set_audio(RerunOBSClient::aEncode, obs_get_audio()); //Pipe OBS sources audio -> aEncode

	//RTMP output handler with default settings
	RerunOBSClient::rtmpOut = obs_output_create("rtmp_output", "RTMP out", obs_data_create(), NULL);
	//Hook encoders up to the output
	obs_output_set_video_encoder(RerunOBSClient::rtmpOut, RerunOBSClient::vEncode);
	obs_output_set_audio_encoder(RerunOBSClient::rtmpOut, RerunOBSClient::aEncode, 0);
}

//Shutdown the OBS subprocess
void RerunOBSClient::shutdown(const Napi::CallbackInfo &info)
{
	obs_shutdown();
	mainSceneObj.Reset();
}

//Start the RTMP output and stream to the target server
void RerunOBSClient::startStream(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	if (info.Length() != 2)
	{
		throw Napi::Error::New(env, "Invalid number of arguments");
	}

	if (!info[0].IsString() || !info[1].IsString())
	{
		throw Napi::Error::New(env, "Invalid arguments");
	}

	std::string address = info[0].As<Napi::String>();
	std::string streamKey = info[1].As<Napi::String>();

	if (obs_output_active(RerunOBSClient::rtmpOut))
	{
		throw Napi::Error::New(env, "stream already active");
	}

	if (RerunOBSClient::rtmpServ == NULL)
	{ //Create an empty RTMP service if one doesn't exist
		obs_data_t *serviceSettings = obs_data_create();
		RerunOBSClient::rtmpServ = obs_service_create("rtmp_custom", "RTMP service", serviceSettings, NULL);
		obs_output_set_service(RerunOBSClient::rtmpOut, RerunOBSClient::rtmpServ); //Hook up RTMP output -> streaming service
	}

	//Update the service settings (TODO: OBS provides preconfigured options for Twitch, Youtube. They should be exposed somehow)
	obs_data_t *serviceSettings = obs_service_get_settings(RerunOBSClient::rtmpServ);
	obs_data_set_string(serviceSettings, "server", address.c_str());
	obs_data_set_string(serviceSettings, "key", streamKey.c_str());
	obs_data_set_bool(serviceSettings, "use_auth", false);

	obs_service_update(RerunOBSClient::rtmpServ, serviceSettings);
	obs_data_release(serviceSettings);

	//Start streaming
	if (!obs_output_start(RerunOBSClient::rtmpOut))
	{
		const char *error = obs_output_get_last_error(RerunOBSClient::rtmpOut);
		throw Napi::Error::New(env, error);
	};
}

void RerunOBSClient::stopStream(const Napi::CallbackInfo &info)
{
	obs_output_stop(RerunOBSClient::rtmpOut);
}

//Create a new scene object
Napi::Value RerunOBSClient::getMainScene(const Napi::CallbackInfo &info)
{
	return mainSceneObj.Value();
}

//Convert a JS map of OBSData into an obs_data object
obs_data_t *RerunOBSClient::createDataFromJS(Napi::Object jsObj)
{
	Napi::Array properties = jsObj.GetPropertyNames();
	obs_data_t *data = obs_data_create();

	for (uint32_t i = 0; i < properties.Length(); i++)
	{
		std::string key = properties.Get(i).As<Napi::String>();
		Napi::Value value = jsObj.Get(key);

		//Verify this object is OBSData
		if (!value.IsObject())
		{
			continue;
		}
		Napi::Object valueObject = value.As<Napi::Object>();
		if (!valueObject.HasOwnProperty("type") || !valueObject.HasOwnProperty("value"))
		{
			std::cout << "createDataFromJS - " << key << " was skipped\n";
			continue;
		}

		//Supported obs_data types
		std::string dataType = valueObject.Get("type").As<Napi::String>();
		if (dataType == "string")
		{
			std::string vString = valueObject.Get("value").As<Napi::String>();
			obs_data_set_string(data, key.c_str(), vString.c_str());
		}
		else if (dataType == "int")
		{
			obs_data_set_int(data, key.c_str(), valueObject.Get("value").As<Napi::Number>().Int32Value());
			//std::cout << "createDataFromJS - Set int " << key.c_str() << " to value " << valueObject.Get("value").As<Napi::Number>().Int32Value() << "\n";
		}
		else if (dataType == "double")
		{
			obs_data_set_double(data, key.c_str(), valueObject.Get("value").As<Napi::Number>().DoubleValue());
		}
		else if (dataType == "bool")
		{
			obs_data_set_bool(data, key.c_str(), valueObject.Get("value").As<Napi::Boolean>());
		}
		else if (dataType == "array")
		{
			Napi::Array jsArray = valueObject.Get("value").As<Napi::Array>();

			//Create the OBS array
			obs_data_array_t *obs_array = obs_data_array_create();

			//Populate the OBS array with OBSData objects from the JS array
			for (uint32_t n = 0; n < jsArray.Length(); n++)
			{
				Napi::Value val = jsArray.Get(n);
				if (val.IsObject())
				{
					obs_data_array_push_back(obs_array, createDataFromJS(val.As<Napi::Object>()));
				}
			}

			//Add the array to the data object
			obs_data_set_array(data, key.c_str(), obs_array);
		}
	}

	return data;
}

//OBS preview window
void RerunOBSClient::openPreviewWindow(const Napi::CallbackInfo &info)
{
	if (!previewWindowOpen) {
		std::thread previewWindowThread(&RerunOBSClient::runPreviewWindow, this);
	}
}

static void renderWindow(void *data, uint32_t cx, uint32_t cy)
{
	obs_render_main_texture();

	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}

static LRESULT CALLBACK sceneProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

void RerunOBSClient::runPreviewWindow() 
{
	std::cout << "Opening preview window...\n";
	HWND windowHandle = NULL;

	//Get the process HINSTANCE
	HINSTANCE hInstance = GetModuleHandle(NULL);

	//Create an empty window
	WNDCLASS wc = {};
	wc.lpszClassName = TEXT("Rerun preview");
	wc.hInstance = hInstance;
	wc.lpfnWndProc = (WNDPROC)sceneProc;

	if (!RegisterClass(&wc)) {
		std::cout << "Failed to register window class\n";
		return;
	}

	windowHandle = CreateWindowEx(
		0,
		TEXT("Rerun preview"),
		TEXT("Rerun preview"),
		WS_OVERLAPPEDWINDOW,

		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (!windowHandle) {
		std::cout << "Failed to create preview window\n";
		return;
	}

	ShowWindow(windowHandle, SW_SHOW);
	previewWindowOpen = true;

	//Create the display inside OBS
	RECT rc;
	GetClientRect(windowHandle, &rc);

	gs_init_data gs_info = {};
	gs_info.cx = rc.right;
	gs_info.cy = rc.bottom;
	gs_info.format = GS_RGBA;
	gs_info.zsformat = GS_ZS_NONE;
	gs_info.window.hwnd = windowHandle;

	obs_display_t* obsDisplay = obs_display_create(&gs_info, 0);

	obs_display_add_draw_callback(obsDisplay, renderWindow, NULL);

	//Pass messages to the window - this loop will exit when the window is closed
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	previewWindowOpen = false;
	std::cout << "Closed preview window\n";
}

//Node native addon API init hook - tell Node which functions we're exporting from this addon
Napi::FunctionReference RerunOBSClient::constructor;
void RerunOBSClient::NapiInit(Napi::Env env, Napi::Object exports)
{
	Napi::HandleScope scope(env);
	Napi::Function constructFunc = DefineClass(env, "OBSClient", {InstanceMethod("init", &RerunOBSClient::init), InstanceMethod("loadModule", &RerunOBSClient::loadModule), InstanceMethod("setupEncoders", &RerunOBSClient::setupEncoders), InstanceMethod("shutdown", &RerunOBSClient::shutdown), InstanceMethod("startStream", &RerunOBSClient::startStream), InstanceMethod("stopStream", &RerunOBSClient::stopStream), InstanceMethod("getMainScene", &RerunOBSClient::getMainScene),

																  InstanceMethod("openPreviewWindow", &RerunOBSClient::openPreviewWindow)});
	RerunOBSClient::constructor = Napi::Persistent(constructFunc);
	RerunOBSClient::constructor.SuppressDestruct();
}