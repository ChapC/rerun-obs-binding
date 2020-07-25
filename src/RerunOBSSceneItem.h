#ifndef RERUN_OBSSCENEITEM_H
#define RERUN_OBSSCENEITEM_H

#include "napi.h"
#include "RerunOBSSource.h"
class RerunOBSSource;
#include "RerunOBSScene.h"
class RerunOBSScene;
#include "obs.h"

class RerunOBSSceneItem : public Napi::ObjectWrap<RerunOBSSceneItem>
{
    public:
        static Napi::FunctionReference constructor;
        static void NapiInit(Napi::Env env, Napi::Object exports);

        RerunOBSSceneItem(const Napi::CallbackInfo& info);

        Napi::Value isVisible(const Napi::CallbackInfo &info);
        void setVisible(const Napi::CallbackInfo &info);

        Napi::Value getSourceNAPI(const Napi::CallbackInfo& info);
        RerunOBSSource* getSource();
        Napi::Value getParentScene(const Napi::CallbackInfo& info);
        obs_sceneitem_t* getSceneItemRef() { return sceneItemRef; }

        void changeOrder(const Napi::CallbackInfo &info);
        void setOrderPosition(const Napi::CallbackInfo &info);

        void stretchToFill();
    private:
        obs_sceneitem_t* sceneItemRef;
        RerunOBSScene* parentScene;
        RerunOBSSource* source;
};

#endif