#include <global_callbacks.hpp>
#include <trace_reader.hpp>

namespace definition
{
    @otf2 for def in defs|global_defs:

    OTF2_CallbackCode
    Global@@def.name@@Cb( void* userData @@def.funcargs()@@ )
    {
        auto tr = static_cast<TraceReader *>(userData);
        @otf2  if def.name == 'Location':
        if (tr->m_locations.size() == tr->m_location_count)
        {
            return OTF2_CALLBACK_INTERRUPT;
        }
        tr->m_locations.push_back(self);
        @otf2 endif
        tr->handler().handleGlobal@@def.name@@(@@def.callargs(leading_comma=False)@@);
        return OTF2_CALLBACK_SUCCESS;
    }

    @otf2 endfor
}