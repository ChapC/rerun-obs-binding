#include "JSEventProvider.h"

Napi::Value JSEventProvider::onNAPI(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 2) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsString() || !info[1].IsFunction()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    std::string eventName = info[0].As<Napi::String>();
    Napi::Function callback = info[1].As<Napi::Function>();

    uint32_t listenerId = on(eventName, Napi::ThreadSafeFunction::New(env, callback, "JSEventProv", 0, 1));
    //Return the listenerId (can be used with off() to cancel this listener)
    return Napi::Number::New(env, listenerId);
}

Napi::Value JSEventProvider::once(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 2) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsString() || !info[1].IsFunction()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    std::string eventName = info[0].As<Napi::String>();
    Napi::Function callback = info[1].As<Napi::Function>();

    uint32_t listenerId = on(eventName, Napi::ThreadSafeFunction::New(env, callback, "JSEventProv", 0, 1), true);
    //Return the listenerId (can be used with off() to cancel this listener)
    return Napi::Number::New(env, listenerId);
}

uint32_t JSEventProvider::on(std::string eventName, Napi::ThreadSafeFunction func, bool once)
{
    uint32_t listenerId = listenerIdCounter++;
    //Add to the listenerId -> event map
    listenerIdMap[listenerId] = eventName;
    //Add to the list of listeners for the event
    listeners[eventName].push_back({ listenerId, func, once });
    return listenerId;
}

void JSEventProvider::offNAPI(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1) {
        throw Napi::Error::New(env, "Invalid number of arguments");
    }

    if (!info[0].IsNumber()) {
        throw Napi::Error::New(env, "Invalid arguments"); 
    }

    uint32_t listenerId = info[0].As<Napi::Number>().Uint32Value();
    off(listenerId);
}

void JSEventProvider::off(uint32_t listenerId) {

    //Find the event that this listener was registered for
    auto idEventPair = listenerIdMap.find(listenerId);
    if (idEventPair == listenerIdMap.end()) {
        return; //This listener is either invalid or has already been cancelled
    }

    std::string eventName = idEventPair->second;
    //Get the list of callbacks registered for this event
    std::vector<JSCallback>& callbacks = listeners.at(eventName);
    for (int i = 0; i < callbacks.size(); i++) {
        if (callbacks[i].listenerId == listenerId) {
            //Remove the target callback
            callbacks.at(i).callbackFunction.Release();
            callbacks.erase(callbacks.begin() + i);
            //Remove this listenerId from the listenerIdMap
            listenerIdMap.erase(listenerId);

            if (callbacks.size() == 0) { //No more listeners for this event
                listeners.erase(eventName);
            }
            break;
        }
    }
}

void JSEventProvider::triggerJSEvent(std::string eventName)
{
    //Find the callbacks registered for this event
    auto eventCBListPair = this->listeners.find(eventName);
    if (eventCBListPair == this->listeners.end()) {
        return; //No callbacks registered for this event
    }

    auto mainThreadCallback = []( Napi::Env env, Napi::Function jsCallback ) {
        jsCallback.Call({});
    };

    std::vector<JSCallback>& callbacks = eventCBListPair->second;
    //Trigger each callback
    for (int i = callbacks.size() - 1; i > -1; i--) {
        JSCallback callback = callbacks.at(i);
        Napi::ThreadSafeFunction func = callback.callbackFunction; //OBS calls signal callbacks on a separate thread, hence the need for acquire and release calls
        func.Acquire();
        func.BlockingCall(mainThreadCallback);
        func.Release();

        if (callback.once)
        {
            off(callback.listenerId);
        }
    }
}