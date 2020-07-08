#include "OBSSource.h"
#include "OBSClient.h"

Napi::Value OBSSource::getName(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    const char *name = obs_source_get_name(this->sourceRef);
    triggerJSEvent("namecheck");
    return Napi::String::New(env, name);
}

Napi::Value OBSSource::isEnabled(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, obs_source_enabled(this->sourceRef));
}

void OBSSource::setEnabled(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 0)
    {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsBoolean())
    {
        throw Napi::Error::New(env, "Invalid arguments");
    }

    Napi::Boolean enabled = info[0].As<Napi::Boolean>();

    obs_source_set_enabled(this->sourceRef, enabled);
}

void OBSSource::updateSettings(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1)
    {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsObject())
    {
        throw Napi::Error::New(env, "Invalid arguments");
    }

    Napi::Object settingsObject = info[0].As<Napi::Object>();

    obs_data_t *updatedSettings = OBSClient::createDataFromJS(settingsObject);
    obs_source_update(this->sourceRef, updatedSettings);

    this->stretchToFill(); //Some sources (like VLC) change their bounds when updated, do not want >:(
}

const vec2 middleScreenPos = {0, 0};
void OBSSource::stretchToFill()
{
    obs_sceneitem_t *sceneItem = obs_scene_find_source(this->parentScene, obs_source_get_name(this->sourceRef));
    obs_sceneitem_set_pos(sceneItem, &middleScreenPos);
    obs_sceneitem_set_bounds_type(sceneItem, OBS_BOUNDS_STRETCH);

    //Get screen height and width
    obs_video_info vInfo = {};
    if (!obs_get_video_info(&vInfo))
    {
        printf("OBSSource::stretchToFill failed, video system not set up");
        return; //Video system not set up
    }
    vec2 fullScreenBounds = {(float)vInfo.base_width, (float)vInfo.base_height};
    obs_sceneitem_set_bounds(sceneItem, &fullScreenBounds);
}

//Accepts source name, source type and source settings
OBSSource::OBSSource(const Napi::CallbackInfo &info) : Napi::ObjectWrap<OBSSource>(info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() != 3)
    {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsObject())
    {
        throw Napi::Error::New(env, "Invalid arguments");
    }

    std::string name = info[0].As<Napi::String>();
    std::string sourceType = info[1].As<Napi::String>();
    Napi::Object settingsObject = info[2].As<Napi::Object>();

    obs_data_t *sourceSettings = OBSClient::createDataFromJS(settingsObject);

    //Create the source
    obs_source_t *source = obs_source_create(sourceType.c_str(), name.c_str(), sourceSettings, NULL);

    if (source == NULL)
    {
        throw Napi::Error::New(env, "source create failed");
    }

    this->sourceRef = source;
}

//Event listeners for this class are hooked up to OBS signals
Napi::Value OBSSource::on(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 2)
    {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsString())
    {
        throw Napi::Error::New(env, "Invalid arguments");
    }

    std::string eventName = info[0].As<Napi::String>();
    signal_handler_t *sourceSignalHandler = obs_source_get_signal_handler(this->sourceRef);

    //Let JSEventProvider store the JS listener
    Napi::Value napiListenerId = JSEventProvider::on(info);
    uint32_t listenerId = napiListenerId.As<Napi::Number>().Uint32Value();

    //Create the signal callback data
    OBSSignalCallbackData* jsCallback = new OBSSignalCallbackData{ eventName, this }; //The OBS signal handler must be static, so a reference to this class instance is required

    //Store the signal callback data in a map so it can be deleted later
    signalCallbackDataMap.insert({ listenerId, jsCallback });

    signal_handler_connect(sourceSignalHandler, eventName.c_str(), obsSignalRepeater, jsCallback);

    return napiListenerId;
}

void OBSSource::off(const Napi::CallbackInfo &info) 
{
    Napi::Env env = info.Env();

    if (info.Length() != 1) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsNumber()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    //Let JSEventProvider do its cleanup
    JSEventProvider::off(info);

    //Delete the callback data for this listener if it still exists
    uint32_t listenerId = info[0].As<Napi::Number>().Uint32Value();

    auto idDataPair = signalCallbackDataMap.find(listenerId);
    if (idDataPair == signalCallbackDataMap.end()) {
        return; //This listener is either invalid or its callback has already been deleted
    }

    OBSSignalCallbackData* callbackData = idDataPair->second;

    //Disconnect the obs signal handler
    signal_handler_t *sourceSignalHandler = obs_source_get_signal_handler(this->sourceRef);
    signal_handler_disconnect(sourceSignalHandler, callbackData->eventName.c_str(), obsSignalRepeater, callbackData);

    signalCallbackDataMap.erase(idDataPair);
    delete callbackData;
}

void OBSSource::obsSignalRepeater(void* customData, calldata_t* signalData)
{
    OBSSignalCallbackData* callbackData = (OBSSignalCallbackData*)customData;
    callbackData->parent->triggerJSEvent(callbackData->eventName);
}

//NAPI initializers

Napi::FunctionReference OBSSource::constructor;

void OBSSource::NapiInit(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);

    Napi::Function constructFunc = DefineClass(env, "OBSScene", {
        InstanceMethod("getName", &OBSSource::getName), 
        InstanceMethod("isEnabled", &OBSSource::isEnabled), 
        InstanceMethod("setEnabled", &OBSSource::setEnabled), 
        InstanceMethod("updateSettings", &OBSSource::updateSettings),
        //JSEventProvider
        InstanceMethod("on", &OBSSource::on), InstanceMethod("off", &OBSSource::off)
    });

    OBSSource::constructor = Napi::Persistent(constructFunc);
    OBSSource::constructor.SuppressDestruct();
}