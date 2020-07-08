#ifndef OBSSCENE_H
#define OBSSCENE_H

#include "napi.h"
#include "obs.h"

//NOTE: Currently this binding doesn't expose SceneItems, instead managing them internally. Rerun only ever uses one scene, so there's no need for the distinction.
class OBSScene : public Napi::ObjectWrap<OBSScene>
{
    public:
        static Napi::FunctionReference constructor;
        static void NapiInit(Napi::Env env, Napi::Object exports);

        OBSScene(const Napi::CallbackInfo& info);

        Napi::Value getName(const Napi::CallbackInfo &info);
        
        Napi::Value addSource(const Napi::CallbackInfo &info);
        void removeSource(const Napi::CallbackInfo &info);

    private:
        obs_scene_t* sceneRef;
        std::vector<Napi::ObjectReference> sourceObjects;
};

#endif