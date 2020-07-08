#ifndef JSEVENTPROV_H
#define JSEVENTPROV_H

#include "napi.h"
#include "map"

struct JSCallback
{
    uint32_t listenerId;
    Napi::ThreadSafeFunction callbackFunction;
};

//Allows callbacks to be sent from C++ to JS code
class JSEventProvider
{
    public:
        Napi::Value on(const Napi::CallbackInfo& info);
        void off(const Napi::CallbackInfo& info);

    protected:
        void triggerJSEvent(std::string eventName);

    private:
        uint32_t listenerIdCounter = 0;
        //Maps listenerId to the event name it's registered for
        std::map<uint32_t, std::string> listenerIdMap;
        //Maps event name to list of listeners
        std::map<std::string, std::vector<JSCallback>> listeners;
};

#endif