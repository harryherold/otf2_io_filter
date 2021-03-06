#pragma once

#include <iostream>
#include <vector>
#include <functional>

extern "C"
{
    #include <otf2/otf2.h>
}

template<typename...ArgTypes>
using FilterCallback = std::function<bool(ArgTypes...args)>;

/*
 *  global definition filter
 */
@otf2 for def in defs|global_defs:

using Global@@def.name@@Filter = FilterCallback<
@@def.attributes[0].type@@
@otf2 for attr in def.attributes[1:]:
, @@attr.type@@
@otf2   if attr is array_attr:
@otf2    for array_attr in attr.array_attributes:
, const @@array_attr.type@@ *
@otf2    endfor
@otf2   endif
@otf2 endfor
>;
@otf2 endfor

/*
 *  local definition filter
 */
@otf2 for def in defs|local_defs:
@otf2 if "MappingTable" == def.name or "ClockOffset" == def.name:

using Local@@def.name@@Filter = FilterCallback<
OTF2_LocationRef
@otf2 for attr in def.attributes:
,@@attr.type@@
@otf2   if attr is array_attr:
@otf2    for array_attr in attr.array_attributes:
, const @@array_attr.type@@ *
@otf2    endfor
@otf2   endif
@otf2 endfor
>;

@otf2   endif
@otf2 endfor

/*
 *  event filter
 */
@otf2 for event in events:

using Event@@event.name@@Filter = FilterCallback<
OTF2_LocationRef
, OTF2_TimeStamp
, OTF2_AttributeList *
@otf2 for attr in event.attributes:
, @@attr.type@@
@otf2   if attr is array_attr:
@otf2    for array_attr in attr.array_attributes:
, const @@array_attr.type@@ *
@otf2    endfor
@otf2   endif
@otf2 endfor
>;

@otf2 endfor

class IFilterCallbacks
{
public:
    struct Callbacks
    {
        /*
         * Global definition callbacks
         */
        @otf2 for def in defs|global_defs:

        Global@@def.name@@Filter global_@@def.lower@@_callback;

        @otf2 endfor
        /*
         * Local definition callbacks
         */
        @otf2 for def in defs|local_defs:
        @otf2 if "MappingTable" == def.name or "ClockOffset" == def.name:

        Local@@def.name@@Filter local_@@def.lower@@_callback;

        @otf2 endif
        @otf2 endfor
        /*
        * Event filter callbacks
        */
        @otf2 for event in events:

        Event@@event.name@@Filter event_@@event.lower@@_callback;

        @otf2 endfor
    };
    virtual Callbacks get_callbacks() = 0;
};

template<typename T>
class Filter
{
public:
    inline void add(T & f)
    {
        m_callbacks.push_back(f);
    }

    template<typename...ArgTypes>
    inline bool process(ArgTypes...args)
    {
        bool b = false;
        for(auto f: m_callbacks)
        {
            b |= f(args...);
        }
        return b;
    }
private:
    std::vector<T> m_callbacks;
};
