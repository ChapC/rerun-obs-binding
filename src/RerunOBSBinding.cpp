#include "napi.h"
#include "RerunOBSClient.h"
#include "RerunOBSScene.h"
#include "RerunOBSSource.h"
#include "RerunOBSSceneItem.h"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    /*
    Exports OBSClient as the "default export", so on the JS side you use
        const OBS = require('bindings')('OBSClient');
        OBS.init();
    */

    RerunOBSClient::NapiInit(env, exports);
    RerunOBSScene::NapiInit(env, exports);
    RerunOBSSource::NapiInit(env, exports);
    RerunOBSSceneItem::NapiInit(env, exports);

    Napi::Object rootObj = RerunOBSClient::constructor.New({});
    return rootObj;
};

NODE_API_MODULE(RerunOBSClient, Init)