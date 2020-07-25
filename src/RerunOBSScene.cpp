#include "RerunOBSScene.h"
#include "RerunOBSSource.h"
#include "RerunOBSSceneItem.h"

//Add a source to a scene, creating a SceneItem
Napi::Value RerunOBSScene::addSource(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (info.Length() != 1) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsObject()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    
    Napi::Object sourceObj = info[0].As<Napi::Object>();
	RerunOBSSource* source = RerunOBSSource::Unwrap(sourceObj);

    //Check that this source isn't in the scene already (Rerun does not allow duplicates)
    if (findSceneItemForSource(source) != NULL) throw Napi::Error::New(env, "The provided source already exists in this scene");

    Napi::Object sceneItemObj = RerunOBSSceneItem::constructor.New({ this->Value(), info[0] });
    RerunOBSSceneItem* sceneItem = RerunOBSSceneItem::Unwrap(sceneItemObj);
    int64_t sceneItemId = obs_sceneitem_get_id(sceneItem->getSceneItemRef());
    sceneItemMap[sceneItemId] = Napi::ObjectReference::New(sceneItemObj, 1); //Keep a ref to this sceneItem

    //Give the source a reference to its sceneitem
    source->setSceneItem(sceneItem);

    sceneItem->stretchToFill();

    return sceneItemObj;
}

//Remove a source from the scene and release its sceneitem
void RerunOBSScene::removeSceneItemNAPI(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    
    if (info.Length() != 1) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsObject()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    Napi::Object sceneItemObj = info[0].As<Napi::Object>();
    RerunOBSSceneItem* sceneItem = RerunOBSSceneItem::Unwrap(sceneItemObj);

    removeSceneItem(sceneItem);
}

void RerunOBSScene::removeSceneItem(RerunOBSSceneItem* sceneItem)
{
    //Remove from the OBS scene
    obs_sceneitem_remove(sceneItem->getSceneItemRef());

    //Remove the source's sceneitem ref since it's no longer attached to a scene
    sceneItem->getSource()->setSceneItem(NULL);

    //Reset the ref for this NAPI object so Node knows it can be destroyed
    auto idItemPair = sceneItemMap.find(obs_sceneitem_get_id(sceneItem->getSceneItemRef()));
    if (idItemPair == sceneItemMap.end()) {
        return; //This sceneitem has already been removed
    }
    
    idItemPair->second.Reset();
    //Remove from the map
    sceneItemMap.erase(idItemPair);
}

Napi::Value RerunOBSScene::findSceneItemForSourceNAPI(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    
    if (info.Length() != 1) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsObject()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    Napi::Object sourceObj = info[1].As<Napi::Object>();
	RerunOBSSource* source = RerunOBSSource::Unwrap(sourceObj);
    
    RerunOBSSceneItem* sceneItem = findSceneItemForSource(source);
    if (sceneItem == NULL) return env.Null();

    return sceneItem->Value();
}

RerunOBSSceneItem* RerunOBSScene::findSceneItemForSource(RerunOBSSource* source)
{
    obs_sceneitem_t* targetSceneItem = obs_scene_find_source(this->sceneRef, obs_source_get_name(source->getSourceRef()));
    if (targetSceneItem == NULL) return NULL;

    int64_t targetId = obs_sceneitem_get_id(targetSceneItem);

    //Find the NAPI object for this sceneitem
    auto idItemPair = sceneItemMap.find(targetId);
    if (idItemPair == sceneItemMap.end()) return NULL;

    return RerunOBSSceneItem::Unwrap(idItemPair->second.Value());
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
        InstanceMethod("removeSceneItem", &RerunOBSScene::removeSceneItemNAPI),
        InstanceMethod("findSceneItemForSource", &RerunOBSScene::findSceneItemForSourceNAPI),
        InstanceMethod("getName", &RerunOBSScene::getName)
    });

    RerunOBSScene::constructor = Napi::Persistent(constructFunc);
    RerunOBSScene::constructor.SuppressDestruct();

    //exports.Set("OBSScene", constructFunc); This exports the constructor. Don't want!
}