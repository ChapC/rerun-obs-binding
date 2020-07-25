#include "RerunOBSSource.h"
#include "RerunOBSClient.h"
#include "RerunOBSSceneItem.h"

Napi::Value RerunOBSSource::getName(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    const char *name = obs_source_get_name(this->sourceRef);
    triggerJSEvent("namecheck");
    return Napi::String::New(env, name);
}

Napi::Value RerunOBSSource::isEnabled(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, obs_source_enabled(this->sourceRef));
}

void RerunOBSSource::setEnabled(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1)
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

void RerunOBSSource::updateSettings(const Napi::CallbackInfo &info)
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

    obs_data_t *updatedSettings = RerunOBSClient::createDataFromJS(settingsObject);
    obs_source_update(this->sourceRef, updatedSettings);

    if (this->sceneItemRef != NULL) {
        this->sceneItemRef->stretchToFill(); //Some sources (like VLC) change their bounds when updated, do not want >:(
    }
}

void RerunOBSSource::restartMedia(const Napi::CallbackInfo &info)
{
    obs_source_media_restart(this->sourceRef);
}

void RerunOBSSource::playMedia(const Napi::CallbackInfo &info)
{
    obs_source_media_play_pause(this->sourceRef, false);
}

void RerunOBSSource::pauseMedia(const Napi::CallbackInfo &info)
{
    obs_source_media_play_pause(this->sourceRef, true);
}

void RerunOBSSource::stopMedia(const Napi::CallbackInfo &info)
{
    obs_source_media_stop(this->sourceRef);
}

//Accepts source name, source type and source settings
RerunOBSSource::RerunOBSSource(const Napi::CallbackInfo &info) : Napi::ObjectWrap<RerunOBSSource>(info)
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

    obs_data_t *sourceSettings = RerunOBSClient::createDataFromJS(settingsObject);

    //Create the source
    obs_source_t* source = obs_source_create(sourceType.c_str(), name.c_str(), sourceSettings, NULL);

    if (source == NULL)
    {
        throw Napi::Error::New(env, "source create failed");
    }

    this->sourceRef = source;
}

//Event listeners for this class are hooked up to OBS signals
Napi::Value RerunOBSSource::onNAPI(const Napi::CallbackInfo &info)
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
    Napi::Value napiListenerId = JSEventProvider::onNAPI(info);
    uint32_t listenerId = napiListenerId.As<Napi::Number>().Uint32Value();

    //Create the signal callback data
    OBSSignalCallbackData* jsCallback = new OBSSignalCallbackData{ eventName, this }; //The OBS signal handler must be static, so a reference to this class instance is required

    //Store the signal callback data in a map so it can be deleted later
    signalCallbackDataMap.insert({ listenerId, jsCallback });

    signal_handler_connect(sourceSignalHandler, eventName.c_str(), obsSignalRepeater, jsCallback);

    return napiListenerId;
}

Napi::Value RerunOBSSource::once(const Napi::CallbackInfo &info)
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
    Napi::Value napiListenerId = JSEventProvider::once(info);
    uint32_t listenerId = napiListenerId.As<Napi::Number>().Uint32Value();

    //Create the signal callback data
    OBSSignalCallbackData* jsCallback = new OBSSignalCallbackData{ eventName, this }; //The OBS signal handler must be static, so a reference to this class instance is required

    //Store the signal callback data in a map so it can be deleted later
    signalCallbackDataMap.insert({ listenerId, jsCallback });

    signal_handler_connect(sourceSignalHandler, eventName.c_str(), obsSignalRepeater, jsCallback);

    return napiListenerId;
}

void RerunOBSSource::off(const Napi::CallbackInfo &info) 
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

void RerunOBSSource::obsSignalRepeater(void* customData, calldata_t* signalData)
{
    OBSSignalCallbackData* callbackData = (OBSSignalCallbackData*)customData;
    callbackData->parent->triggerJSEvent(callbackData->eventName);
}

//NAPI initializers

Napi::FunctionReference RerunOBSSource::constructor;

void RerunOBSSource::NapiInit(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);

    Napi::Function constructFunc = DefineClass(env, "OBSSource", {
        InstanceMethod("getName", &RerunOBSSource::getName), 
        InstanceMethod("isEnabled", &RerunOBSSource::isEnabled), 
        InstanceMethod("setEnabled", &RerunOBSSource::setEnabled),
        InstanceMethod("updateSettings", &RerunOBSSource::updateSettings),
        InstanceMethod("restartMedia", &RerunOBSSource::restartMedia),
        InstanceMethod("playMedia", &RerunOBSSource::playMedia),
        InstanceMethod("pauseMedia", &RerunOBSSource::pauseMedia),
        InstanceMethod("stopMedia", &RerunOBSSource::stopMedia),
        //JSEventProvider
        InstanceMethod("on", &RerunOBSSource::onNAPI), InstanceMethod("once", &RerunOBSSource::once), InstanceMethod("off", &RerunOBSSource::off)
    });

    RerunOBSSource::constructor = Napi::Persistent(constructFunc);
    RerunOBSSource::constructor.SuppressDestruct();
}