#include "RerunOBSScene.h"
#include "RerunOBSSource.h"

//Create a source and add it to the scene
Napi::Value RerunOBSScene::addSource(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (info.Length() != 3) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsString() | !info[1].IsString() | !info[2].IsObject()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    Napi::Object sourceWrap = RerunOBSSource::constructor.New({info[0], info[1], info[2]});
    
    //Add to this scene
    RerunOBSSource* sourceObj = RerunOBSSource::Unwrap(sourceWrap);
    obs_source_t* source = sourceObj->getSourceRef();
    obs_sceneitem_t* sceneItem = obs_scene_add(this->sceneRef, source);

    sourceObj->setSceneItem(sceneItem);

    sourceObj->stretchToFill(); //Rerun needs all sources to be stretched to fill */
    
    //Store a ref to the source so Node doesn't destroy it
    sourceObjects.push_back(Napi::ObjectReference::New(sourceWrap, 1));
    return sourceWrap;
}

//Remove a source from the scene and release it
void RerunOBSScene::removeSource(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    
    if (info.Length() != 1) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsObject()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    Napi::Object sourceWrap = info[0].As<Napi::Object>();
    RerunOBSSource* sourceObj = RerunOBSSource::Unwrap(sourceWrap);
    obs_source_remove(sourceObj->getSourceRef());
    obs_source_release(sourceObj->getSourceRef());

    //Reset the ref for this object so Node knows it can be destroyed
    for (int i = 0; i < sourceObjects.size(); i++) {
        if (sourceObjects[i].Value() == sourceWrap) {
            sourceObjects[i].Reset();
            sourceObjects.erase(sourceObjects.begin() + i);
            break;
        }
    }
}

Napi::Value RerunOBSScene::getName(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    obs_source_t* scene_source = obs_scene_get_source(sceneRef);
    const char* source_name = obs_source_get_name(scene_source);
    return Napi::String::New(env, source_name);
}

//Accepts sceneName: string, outputChannel: number (0-63)
RerunOBSScene::RerunOBSScene(const Napi::CallbackInfo& info) : Napi::ObjectWrap<RerunOBSScene>(info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    
    if (info.Length() != 2) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsString() || !info[1].IsNumber()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    std::string sceneName = info[0].As<Napi::String>();
    int outChannel = info[1].As<Napi::Number>();

    //Create the scene
    obs_scene_t* scene = obs_scene_create(sceneName.c_str());
    this->sceneRef = scene;

    //Assign it to an output channel
    obs_source_t* sceneSource = obs_scene_get_source(scene);
    obs_set_output_source(outChannel, sceneSource);
}

//NAPI initializers

Napi::FunctionReference RerunOBSScene::constructor;

void RerunOBSScene::NapiInit(Napi::Env env, Napi::Object exports) 
{
    Napi::HandleScope scope(env);

    Napi::Function constructFunc = DefineClass(env, "OBSScene", {
        InstanceMethod("addSource", &RerunOBSScene::addSource),
        InstanceMethod("removeSource", &RerunOBSScene::removeSource),
        InstanceMethod("getName", &RerunOBSScene::getName)
    });

    RerunOBSScene::constructor = Napi::Persistent(constructFunc);
    RerunOBSScene::constructor.SuppressDestruct();

    //exports.Set("OBSScene", constructFunc); This exports the constructor. Don't want!
}