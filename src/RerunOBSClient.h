#ifndef RERUN_OBSCLIENT_H
#define RERUN_OBSCLIENT_H

#include "napi.h"
#include "RerunOBSScene.h"
#include <atomic>

class RerunOBSClient : public Napi::ObjectWrap<RerunOBSClient>
{
    public:
        static Napi::FunctionReference constructor;
        static void NapiInit(Napi::Env env, Napi::Object exports);
        
        RerunOBSClient(const Napi::CallbackInfo& info);

        Napi::Value init(const Napi::CallbackInfo& info);
        void loadModule(const Napi::CallbackInfo& info);
        void setupEncoders(const Napi::CallbackInfo& info);
        void shutdown(const Napi::CallbackInfo& info);

        void startStream(const Napi::CallbackInfo& info);
        void stopStream(const Napi::CallbackInfo& info);

        Napi::Value getMainScene(const Napi::CallbackInfo &info);

        static obs_data_t* createDataFromJS(Napi::Object jsObj);

        void openPreviewWindow(const Napi::CallbackInfo& info);
        void closePreviewWindow(const Napi::CallbackInfo& info);
            
    private: //Rerun only uses one output, service, audio encoder, video encoder and scene
        obs_output_t* rtmpOut = NULL;
        obs_service_t* rtmpServ = NULL;
        obs_encoder_t* vEncode = NULL;
        obs_encoder_t* aEncode = NULL;

        std::atomic<bool> previewWindowOpen = false;
        void runPreviewWindow();

        Napi::ObjectReference mainSceneObj;
};

#endif