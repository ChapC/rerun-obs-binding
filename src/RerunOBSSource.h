#ifndef RERUN_OBSSOURCE_H
#define RERUN_OBSSOURCE_H

#include "napi.h"
#include "obs.h"
#include "RerunOBSScene.h"
#include "JSEventProvider.h"
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
    Napi::Value isVisible(const Napi::CallbackInfo &info);
    void setVisible(const Napi::CallbackInfo &info);

    void updateSettings(const Napi::CallbackInfo &info);

    obs_source_t *getSourceRef() { return this->sourceRef; }
    void setSceneItem(obs_sceneitem_t *si) { this->sceneItemRef = si; }
    void stretchToFill();

    void changeOrder(const Napi::CallbackInfo &info);
    void restartMedia(const Napi::CallbackInfo &info);
    void playMedia(const Napi::CallbackInfo &info);
    void pauseMedia(const Napi::CallbackInfo &info);
    void stopMedia(const Napi::CallbackInfo &info);

private:
    obs_source_t* sourceRef;
    obs_sceneitem_t* sceneItemRef;

    //JSEventProvider is extended to provide access to OBS signals
    std::unordered_map<uint32_t, OBSSignalCallbackData*> signalCallbackDataMap; //Maps JSEventListener ID to OBSSignalCallbackData

    Napi::Value on(const Napi::CallbackInfo &info);
    void off(const Napi::CallbackInfo &info);

    static void obsSignalRepeater(void *jsEventData, calldata_t *cd);
};

struct OBSSignalCallbackData
{
    std::string eventName;
    RerunOBSSource* parent;
    uint32_t* listenerIdPtr;
};

#endif