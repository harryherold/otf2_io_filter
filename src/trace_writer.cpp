#include <cassert>
#include <trace_writer.hpp>

OTF2_FlushType pre_flush(void *userData, OTF2_FileType fileType,
                         OTF2_LocationRef location, void *callerData,
                         bool final) {
    return OTF2_FLUSH;
}

OTF2_FlushCallbacks TraceWriter::m_flush_callbacks = {
    .otf2_pre_flush = pre_flush, .otf2_post_flush = nullptr};

void delete_event_writer(OTF2_EvtWriter *writer, OTF2_Archive *archive) {
    if (nullptr != archive && nullptr != writer) {
        OTF2_Archive_CloseEvtWriter(archive, writer);
        OTF2_Archive_CloseEvtFiles(archive);
    }
}

void delete_archive(OTF2_Archive *archive) {
    if (nullptr != archive) {
        OTF2_Archive_Close(archive);
    }
}

TraceWriter::TraceWriter(const std::string &path)
    : m_archive(nullptr, delete_archive),
      m_event_writer(nullptr,
                     [this](OTF2_EvtWriter *writer) {
                         delete_event_writer(writer, m_archive.get());
                     })
{
    auto *archive =
        OTF2_Archive_Open(path.c_str(), "trace", OTF2_FILEMODE_WRITE,
                          1024 * 1024 /* event chunk size */,
                          4 * 1024 * 1024 /* def chunk size */,
                          OTF2_SUBSTRATE_POSIX, OTF2_COMPRESSION_NONE);

    OTF2_Archive_SetFlushCallbacks(archive, &m_flush_callbacks, nullptr);
    OTF2_Archive_SetSerialCollectiveCallbacks(archive);
    m_archive.reset(archive);

    OTF2_Archive_OpenEvtFiles(archive);
    m_event_writer.reset(OTF2_Archive_GetEvtWriter(archive, 0));

    m_def_writer = OTF2_Archive_GetGlobalDefWriter(archive);
    assert(m_def_writer);
}

TraceWriter::~TraceWriter() {}

void TraceWriter::writeClockProperties(uint64_t timerResolution,
                                       uint64_t globalOffset,
                                       uint64_t traceLength)
{
    OTF2_GlobalDefWriter_WriteClockProperties(m_def_writer,
                                              timerResolution,
                                              globalOffset,
                                              traceLength);
}

void TraceWriter::writeSystemTreeNode(OTF2_SystemTreeNodeRef self,
                                      OTF2_StringRef name,
                                      OTF2_StringRef className,
                                      OTF2_SystemTreeNodeRef parent) 
{
    OTF2_GlobalDefWriter_WriteSystemTreeNode(m_def_writer,
                                             self,
                                             name,
                                             className,
                                             parent);
}

void TraceWriter::writeLocationGroup(OTF2_LocationGroupRef self,
                                     OTF2_StringRef name,
                                     OTF2_LocationGroupType locationGroupType,
                                     OTF2_SystemTreeNodeRef systemTreeParent) 
{
    OTF2_GlobalDefWriter_WriteLocationGroup(m_def_writer,
                                            self,
                                            name,
                                            locationGroupType,
                                            systemTreeParent);
}

void TraceWriter::writeLocation(OTF2_LocationRef self, 
                                OTF2_StringRef name,
                                OTF2_LocationType locationType,
                                uint64_t numberOfEvents,
                                OTF2_LocationGroupRef locationGroup) 
{
    OTF2_GlobalDefWriter_WriteLocation(m_def_writer,
                                       self,
                                       name,
                                       locationType,
                                       numberOfEvents,
                                       locationGroup);
}

void
TraceWriter::writeString (OTF2_StringRef self,
		                  const char *  string)
{
    OTF2_GlobalDefWriter_WriteString(m_def_writer,
                                     self,
                                     string);
}

void
TraceWriter::writerEnterEvent(OTF2_LocationRef    location,
                              OTF2_AttributeList * attributeList,
                              OTF2_TimeStamp  	   time,
                              OTF2_RegionRef  	   region)
{
    auto writer = OTF2_Archive_GetEvtWriter(m_archive.get(), location);
    OTF2_EvtWriter_Enter(writer,
                         attributeList,
                         time,
                         region);
}

void
TraceWriter::writerLeaveEvent(OTF2_LocationRef    location,
                              OTF2_AttributeList * attributeList,
                              OTF2_TimeStamp  	   time,
                              OTF2_RegionRef  	   region)
{
    auto writer = OTF2_Archive_GetEvtWriter(m_archive.get(), location);
    OTF2_EvtWriter_Leave(writer,
                         attributeList,
                         time,
                         region);    
}