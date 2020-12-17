#ifndef RERUN_JSEVENTPROV_H
#define RERUN_JSEVENTPROV_H

#include "napi.h"
#include "map"

struct JSCallback
{
    uint32_t listenerId;
    Napi::ThreadSafeFunction callbackFunction;
    bool once;
};

//Allows callbacks to be sent from C++ to JS code
class JSEventProvider
{
    public:
        Napi::Value onNAPI(const Napi::CallbackInfo& info);
        uint32_t on(std::string eventName, Napi::ThreadSafeFunction func, bool once = false);
        void offNAPI(const Napi::CallbackInfo& info);
        void off(uint32_t listenerId);
        Napi::Value once(const Napi::CallbackInfo& info);

        void triggerJSEvent(std::string eventName);

    private:
        uint32_t listenerIdCounter = 0;
        //Maps listenerId to the event name it's registered for
        std::map<uint32_t, std::string> listenerIdMap;
        //Maps event name to list of listeners
        std::map<std::string, std::vector<JSCallback>> listeners;
};

//Very similar to JSEventProvider except uses uint32s for event keys instead of strings
class JSEventProviderIntKeyed
{
    public:
        Napi::Value onNAPI(const Napi::CallbackInfo& info);
        uint32_t on(uint32_t eventKey, Napi::ThreadSafeFunction func, bool once = false);
        void offNAPI(const Napi::CallbackInfo& info);
        void off(uint32_t listenerId);
        Napi::Value once(const Napi::CallbackInfo& info);

        void triggerJSEvent(uint32_t eventKey);

        //Maps event key to list of listeners
        std::map<uint32_t, std::vector<JSCallback>> listeners;
    private:
        uint32_t listenerIdCounter = 0;
        //Maps listenerId to the event key it's registered for
        std::map<uint32_t, uint32_t> listenerIdMap;
};

#endif