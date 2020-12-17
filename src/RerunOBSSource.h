#ifndef RERUN_OBSSOURCE_H
#define RERUN_OBSSOURCE_H

#include "napi.h"
#include "obs.h"
#include "JSEventProvider.h"
#include "RerunOBSSceneItem.h"
class RerunOBSSceneItem;
#include <unordered_map>

struct OBSSignalCallbackData;

class RerunOBSSource : public Napi::ObjectWrap<RerunOBSSource>, public JSEventProvider
{
public:
    static Napi::FunctionReference constructor;
    static void NapiInit(Napi::Env env, Napi::Object exports);

    RerunOBSSource(const Napi::CallbackInfo &info);

    Napi::Value getName(const Napi::CallbackInfo &info);

    Napi::Value isEnabled(const Napi::CallbackInfo &info);
    void setEnabled(const Napi::CallbackInfo &info);

    void updateSettings(const Napi::CallbackInfo &info);

    obs_source_t* getSourceRef() { return this->sourceRef; }
    void setSceneItem(RerunOBSSceneItem* sceneItem) { this->sceneItemRef = sceneItem; }

    void playMedia(const Napi::CallbackInfo &info);
    void pauseMedia(const Napi::CallbackInfo &info);
    void stopMedia(const Napi::CallbackInfo &info);
    void restartMedia(const Napi::CallbackInfo &info);
    Napi::Value getMediaTime(const Napi::CallbackInfo &info);

    Napi::Value onceMediaTime(const Napi::CallbackInfo &info);
    void offMediaTime(const Napi::CallbackInfo &info);

private:
    obs_source_t* sourceRef;
    RerunOBSSceneItem* sceneItemRef = NULL; //Rerun only uses one scene and doesn't support source duplication, so there will only ever be one SceneItem per source
    
    //JSEventProvider is extended to provide access to OBS signals
    std::unordered_map<uint32_t, OBSSignalCallbackData*> signalCallbackDataMap; //Maps JSEventListener ID to OBSSignalCallbackData

    Napi::Value onNAPI(const Napi::CallbackInfo &info);
    Napi::Value once(const Napi::CallbackInfo &info);
    void off(const Napi::CallbackInfo &info);

    static void obsSignalRepeater(void *jsEventData, calldata_t *cd);

    //To facilitate onMediaTime callbacks, we add an audio_capture callback and check the media time whenever new audio data comes in. It's a hack, but I haven't thought of anything better yet.
    JSEventProviderIntKeyed mediaTimeEventProvider;
    bool mediaTimeCallbackActive = false;
    static void mediaTimeAudioCallback(void *param, obs_source_t* source, const struct audio_data* audio_data, bool muted);
};

struct OBSSignalCallbackData
{
    std::string eventName;
    RerunOBSSource* parent;
    uint32_t* listenerIdPtr;
};

#endif