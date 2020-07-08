#ifndef OBSCLIENT_H
#define OBSCLIENT_H

#include "napi.h"
#include "OBSScene.h"

class OBSClient : public Napi::ObjectWrap<OBSClient>
{
    public:
        static Napi::FunctionReference constructor;
        static void NapiInit(Napi::Env env, Napi::Object exports);
        
        OBSClient(const Napi::CallbackInfo& info);

        Napi::Value init(const Napi::CallbackInfo& info);
        void loadModule(const Napi::CallbackInfo& info);
        void setupEncoders(const Napi::CallbackInfo& info);
        void shutdown(const Napi::CallbackInfo& info);

        void startStream(const Napi::CallbackInfo& info);
        void stopStream(const Napi::CallbackInfo& info);

        Napi::Value getMainScene(const Napi::CallbackInfo &info);

        static obs_data_t* createDataFromJS(Napi::Object jsObj);
            
    private: //Rerun only uses one output, service, audio encoder, video encoder and scene
        obs_output_t* rtmpOut = NULL;
        obs_service_t* rtmpServ = NULL;
        obs_encoder_t* vEncode = NULL;
        obs_encoder_t* aEncode = NULL;

        Napi::ObjectReference mainSceneObj;
};

#endif