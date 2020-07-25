#include "RerunOBSSceneItem.h"

Napi::Value RerunOBSSceneItem::isVisible(const Napi::CallbackInfo &info)
{
    return Napi::Boolean::New(info.Env(), obs_sceneitem_visible(this->sceneItemRef));
}

void RerunOBSSceneItem::setVisible(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1)
    {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsBoolean())
    {
        throw Napi::Error::New(env, "Invalid arguments");
    }

    Napi::Boolean visible = info[0].As<Napi::Boolean>();

    obs_sceneitem_set_visible(this->sceneItemRef, visible);
}

Napi::Value RerunOBSSceneItem::getSourceNAPI(const Napi::CallbackInfo& info)
{
    return source->Value();
}

RerunOBSSource* RerunOBSSceneItem::getSource()
{
    return source;
}

Napi::Value RerunOBSSceneItem::getParentScene(const Napi::CallbackInfo& info)
{
    return parentScene->Value();
}

const vec2 middleScreenPos = {0, 0};
void RerunOBSSceneItem::stretchToFill()
{
    obs_sceneitem_set_pos(this->sceneItemRef, &middleScreenPos);
    obs_sceneitem_set_bounds_type(this->sceneItemRef, OBS_BOUNDS_STRETCH);

    //Get screen height and width
    obs_video_info vInfo = {};
    if (!obs_get_video_info(&vInfo))
    {
        printf("OBSSource::stretchToFill failed, video system not set up");
        return; //Video system not set up
    }
    vec2 fullScreenBounds = {(float)vInfo.base_width, (float)vInfo.base_height};
    obs_sceneitem_set_bounds(this->sceneItemRef, &fullScreenBounds);
}

//Accepts scene and source
RerunOBSSceneItem::RerunOBSSceneItem(const Napi::CallbackInfo &info) : Napi::ObjectWrap<RerunOBSSceneItem>(info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() != 2)
    {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsObject() || !info[1].IsObject())
    {
        throw Napi::Error::New(env, "Invalid arguments");
    }

    Napi::Object sceneObj = info[0].As<Napi::Object>();
	RerunOBSScene* scene = RerunOBSScene::Unwrap(sceneObj);

    Napi::Object sourceObj = info[1].As<Napi::Object>();
	RerunOBSSource* source = RerunOBSSource::Unwrap(sourceObj);

    obs_sceneitem_t* sceneItem = obs_scene_add(scene->getSceneRef(), source->getSourceRef());
    this->sceneItemRef = sceneItem;
    this->parentScene = scene;
    this->source = source;
}

//NAPI
Napi::FunctionReference RerunOBSSceneItem::constructor;

void RerunOBSSceneItem::NapiInit(Napi::Env env, Napi::Object exports) 
{
    Napi::HandleScope scope(env);

    Napi::Function constructFunc = DefineClass(env, "OBSSceneItem", {
        InstanceMethod("isVisible", &RerunOBSSceneItem::isVisible),
        InstanceMethod("setVisible", &RerunOBSSceneItem::setVisible),
        InstanceMethod("getSource", &RerunOBSSceneItem::getSourceNAPI),
        InstanceMethod("getParentScene", &RerunOBSSceneItem::getParentScene)
    });

    RerunOBSSceneItem::constructor = Napi::Persistent(constructFunc);
    RerunOBSSceneItem::constructor.SuppressDestruct();

    //exports.Set("OBSScene", constructFunc); This exports the constructor. Don't want!
}