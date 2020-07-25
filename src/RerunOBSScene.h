#ifndef RERUN_OBSSCENE_H
#define RERUN_OBSSCENE_H

#include "napi.h"
#include "obs.h"
#include "RerunOBSSceneItem.h"
class RerunOBSSceneItem;
#include <map>

class RerunOBSScene : public Napi::ObjectWrap<RerunOBSScene>
{
    public:
        static Napi::FunctionReference constructor;
        static void NapiInit(Napi::Env env, Napi::Object exports);

        RerunOBSScene(const Napi::CallbackInfo& info);

        Napi::Value getName(const Napi::CallbackInfo &info);
        obs_scene_t* getSceneRef() { return sceneRef; }
        
        Napi::Value addSource(const Napi::CallbackInfo &info);
        void removeSceneItemNAPI(const Napi::CallbackInfo &info);
        void removeSceneItem(RerunOBSSceneItem* sceneItem);

        Napi::Value findSceneItemForSourceNAPI(const Napi::CallbackInfo &info);
        RerunOBSSceneItem* findSceneItemForSource(RerunOBSSource* source);

    private:
        obs_scene_t* sceneRef;
        std::map<int64_t, Napi::ObjectReference> sceneItemMap; //Maps OBS sceneitem IDs to RerunOBSSceneItem references
};

#endif