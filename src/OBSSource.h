#ifndef OBSSOURCE_H
#define OBSSOURCE_H

#include "napi.h"
#include "obs.h"
#include "OBSScene.h"
#include "JSEventProvider.h"
#include <unordered_map>

struct OBSSignalCallbackData;

class OBSSource : public Napi::ObjectWrap<OBSSource>, public JSEventProvider
{
public:
    static Napi::FunctionReference constructor;
    static void NapiInit(Napi::Env env, Napi::Object exports);

    OBSSource(const Napi::CallbackInfo &info);
    ~OBSSource();

    Napi::Value getName(const Napi::CallbackInfo &info);

    Napi::Value isEnabled(const Napi::CallbackInfo &info);
    void setEnabled(const Napi::CallbackInfo &info);

    void updateSettings(const Napi::CallbackInfo &info);

    obs_source_t *getSourceRef() { return this->sourceRef; }
    void setParentScene(obs_scene_t *parent) { this->parentScene = parent; }
    void stretchToFill();

private:
    obs_source_t *sourceRef;
    obs_scene_t *parentScene;

    //JSEventProvider is extended to provide access to OBS signals
    std::unordered_map<uint32_t, OBSSignalCallbackData*> signalCallbackDataMap; //Maps JSEventListener ID to OBSSignalCallbackData

    Napi::Value on(const Napi::CallbackInfo &info);
    //Napi::Value off(const Napi::CallbackInfo &info);

    static void obsSignalRepeater(void *jsEventData, calldata_t *cd);
};

struct OBSSignalCallbackData
{
    std::string eventName;
    OBSSource* parent;
    uint32_t* listenerIdPtr;
};

#endif