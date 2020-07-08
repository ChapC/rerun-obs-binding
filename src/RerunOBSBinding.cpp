#include "napi.h"
#include "OBSClient.h"
#include "OBSScene.h"
#include "OBSSource.h"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    /*
    Exports OBSClient as the "default export", so on the JS side you use
        const OBS = require('bindings')('OBSClient');
        OBS.init();
    */

    OBSClient::NapiInit(env, exports);
    OBSScene::NapiInit(env, exports);
    OBSSource::NapiInit(env, exports);

    Napi::Object rootObj = OBSClient::constructor.New({});
    return rootObj;
};

NODE_API_MODULE(OBSClient, Init)