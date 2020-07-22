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
    }
}

void delete_archive(OTF2_Archive *archive) {
    if (nullptr != archive) {
        OTF2_Archive_Close(archive);
    }
}

TraceWriter::TraceWriter(const std::string &path)
    : m_archive(nullptr, delete_archive) {
    auto *archive =
        OTF2_Archive_Open(path.c_str(), "trace", OTF2_FILEMODE_WRITE,
                          1024 * 1024 /* event chunk size */,
                          4 * 1024 * 1024 /* def chunk size */,
                          OTF2_SUBSTRATE_POSIX, OTF2_COMPRESSION_NONE);

    OTF2_Archive_SetFlushCallbacks(archive, &m_flush_callbacks, nullptr);
    OTF2_Archive_SetSerialCollectiveCallbacks(archive);
    m_archive.reset(archive);

    OTF2_Archive_OpenEvtFiles(archive);

    m_def_writer = OTF2_Archive_GetGlobalDefWriter(archive);
    assert(m_def_writer);
}

TraceWriter::~TraceWriter() {
    OTF2_Archive_CloseDefFiles(m_archive.get());
    for (auto location : m_locations) {
        OTF2_DefWriter *def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), location);
        OTF2_Archive_CloseDefWriter(m_archive.get(), def_writer);
    }
    OTF2_Archive_CloseEvtFiles(m_archive.get());
}

void TraceWriter::handleGlobalClockProperties(uint64_t timerResolution,
                                              uint64_t globalOffset,
                                              uint64_t traceLength) {

    bool filter_out = m_global_ClockProperties_filter.process(
        timerResolution, globalOffset, traceLength);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteClockProperties(m_def_writer, timerResolution,
                                                  globalOffset, traceLength);
    }
}

void TraceWriter::handleGlobalParadigm(OTF2_Paradigm paradigm,
                                       OTF2_StringRef name,
                                       OTF2_ParadigmClass paradigmClass) {

    bool filter_out =
        m_global_Paradigm_filter.process(paradigm, name, paradigmClass);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteParadigm(m_def_writer, paradigm, name,
                                           paradigmClass);
    }
}

void TraceWriter::handleGlobalParadigmProperty(OTF2_Paradigm paradigm,
                                               OTF2_ParadigmProperty property,
                                               OTF2_Type type,
                                               OTF2_AttributeValue value) {

    bool filter_out = m_global_ParadigmProperty_filter.process(
        paradigm, property, type, value);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteParadigmProperty(m_def_writer, paradigm,
                                                   property, type, value);
    }
}

void TraceWriter::handleGlobalIoParadigm(
    OTF2_IoParadigmRef self, OTF2_StringRef identification, OTF2_StringRef name,
    OTF2_IoParadigmClass ioParadigmClass, OTF2_IoParadigmFlag ioParadigmFlags,
    uint8_t numberOfProperties, const OTF2_IoParadigmProperty *properties,
    const OTF2_Type *types, const OTF2_AttributeValue *values) {

    bool filter_out = m_global_IoParadigm_filter.process(
        self, identification, name, ioParadigmClass, ioParadigmFlags,
        numberOfProperties, properties, types, values);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteIoParadigm(
            m_def_writer, self, identification, name, ioParadigmClass,
            ioParadigmFlags, numberOfProperties, properties, types, values);
    }
}

void TraceWriter::handleGlobalString(OTF2_StringRef self, const char *string) {

    bool filter_out = m_global_String_filter.process(self, string);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteString(m_def_writer, self, string);
    }
}

void TraceWriter::handleGlobalAttribute(OTF2_AttributeRef self,
                                        OTF2_StringRef name,
                                        OTF2_StringRef description,
                                        OTF2_Type type) {

    bool filter_out =
        m_global_Attribute_filter.process(self, name, description, type);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteAttribute(m_def_writer, self, name,
                                            description, type);
    }
}

void TraceWriter::handleGlobalSystemTreeNode(OTF2_SystemTreeNodeRef self,
                                             OTF2_StringRef name,
                                             OTF2_StringRef className,
                                             OTF2_SystemTreeNodeRef parent) {

    bool filter_out =
        m_global_SystemTreeNode_filter.process(self, name, className, parent);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteSystemTreeNode(m_def_writer, self, name,
                                                 className, parent);
    }
}

void TraceWriter::handleGlobalLocationGroup(
    OTF2_LocationGroupRef self, OTF2_StringRef name,
    OTF2_LocationGroupType locationGroupType,
    OTF2_SystemTreeNodeRef systemTreeParent) {

    bool filter_out = m_global_LocationGroup_filter.process(
        self, name, locationGroupType, systemTreeParent);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteLocationGroup(
            m_def_writer, self, name, locationGroupType, systemTreeParent);
    }
}

void TraceWriter::handleGlobalLocation(OTF2_LocationRef self,
                                       OTF2_StringRef name,
                                       OTF2_LocationType locationType,
                                       uint64_t numberOfEvents,
                                       OTF2_LocationGroupRef locationGroup) {
    m_locations.insert(self);

    bool filter_out = m_global_Location_filter.process(
        self, name, locationType, numberOfEvents, locationGroup);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteLocation(m_def_writer, self, name,
                                           locationType, numberOfEvents,
                                           locationGroup);
    }
}

void TraceWriter::handleGlobalRegion(
    OTF2_RegionRef self, OTF2_StringRef name, OTF2_StringRef canonicalName,
    OTF2_StringRef description, OTF2_RegionRole regionRole,
    OTF2_Paradigm paradigm, OTF2_RegionFlag regionFlags,
    OTF2_StringRef sourceFile, uint32_t beginLineNumber,
    uint32_t endLineNumber) {

    bool filter_out = m_global_Region_filter.process(
        self, name, canonicalName, description, regionRole, paradigm,
        regionFlags, sourceFile, beginLineNumber, endLineNumber);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteRegion(
            m_def_writer, self, name, canonicalName, description, regionRole,
            paradigm, regionFlags, sourceFile, beginLineNumber, endLineNumber);
    }
}

void TraceWriter::handleGlobalCallsite(OTF2_CallsiteRef self,
                                       OTF2_StringRef sourceFile,
                                       uint32_t lineNumber,
                                       OTF2_RegionRef enteredRegion,
                                       OTF2_RegionRef leftRegion) {

    bool filter_out = m_global_Callsite_filter.process(
        self, sourceFile, lineNumber, enteredRegion, leftRegion);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteCallsite(m_def_writer, self, sourceFile,
                                           lineNumber, enteredRegion,
                                           leftRegion);
    }
}

void TraceWriter::handleGlobalCallpath(OTF2_CallpathRef self,
                                       OTF2_CallpathRef parent,
                                       OTF2_RegionRef region) {

    bool filter_out = m_global_Callpath_filter.process(self, parent, region);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteCallpath(m_def_writer, self, parent, region);
    }
}

void TraceWriter::handleGlobalGroup(OTF2_GroupRef self, OTF2_StringRef name,
                                    OTF2_GroupType groupType,
                                    OTF2_Paradigm paradigm,
                                    OTF2_GroupFlag groupFlags,
                                    uint32_t numberOfMembers,
                                    const uint64_t *members) {

    bool filter_out = m_global_Group_filter.process(
        self, name, groupType, paradigm, groupFlags, numberOfMembers, members);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteGroup(m_def_writer, self, name, groupType,
                                        paradigm, groupFlags, numberOfMembers,
                                        members);
    }
}

void TraceWriter::handleGlobalMetricMember(
    OTF2_MetricMemberRef self, OTF2_StringRef name, OTF2_StringRef description,
    OTF2_MetricType metricType, OTF2_MetricMode metricMode, OTF2_Type valueType,
    OTF2_Base base, int64_t exponent, OTF2_StringRef unit) {

    bool filter_out = m_global_MetricMember_filter.process(
        self, name, description, metricType, metricMode, valueType, base,
        exponent, unit);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteMetricMember(
            m_def_writer, self, name, description, metricType, metricMode,
            valueType, base, exponent, unit);
    }
}

void TraceWriter::handleGlobalMetricClass(
    OTF2_MetricRef self, uint8_t numberOfMetrics,
    const OTF2_MetricMemberRef *metricMembers,
    OTF2_MetricOccurrence metricOccurrence, OTF2_RecorderKind recorderKind) {

    bool filter_out = m_global_MetricClass_filter.process(
        self, numberOfMetrics, metricMembers, metricOccurrence, recorderKind);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteMetricClass(m_def_writer, self,
                                              numberOfMetrics, metricMembers,
                                              metricOccurrence, recorderKind);
    }
}

void TraceWriter::handleGlobalMetricInstance(OTF2_MetricRef self,
                                             OTF2_MetricRef metricClass,
                                             OTF2_LocationRef recorder,
                                             OTF2_MetricScope metricScope,
                                             uint64_t scope) {

    bool filter_out = m_global_MetricInstance_filter.process(
        self, metricClass, recorder, metricScope, scope);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteMetricInstance(
            m_def_writer, self, metricClass, recorder, metricScope, scope);
    }
}

void TraceWriter::handleGlobalComm(OTF2_CommRef self, OTF2_StringRef name,
                                   OTF2_GroupRef group, OTF2_CommRef parent) {

    bool filter_out = m_global_Comm_filter.process(self, name, group, parent);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteComm(m_def_writer, self, name, group, parent);
    }
}

void TraceWriter::handleGlobalParameter(OTF2_ParameterRef self,
                                        OTF2_StringRef name,
                                        OTF2_ParameterType parameterType) {

    bool filter_out =
        m_global_Parameter_filter.process(self, name, parameterType);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteParameter(m_def_writer, self, name,
                                            parameterType);
    }
}

void TraceWriter::handleGlobalRmaWin(OTF2_RmaWinRef self, OTF2_StringRef name,
                                     OTF2_CommRef comm) {

    bool filter_out = m_global_RmaWin_filter.process(self, name, comm);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteRmaWin(m_def_writer, self, name, comm);
    }
}

void TraceWriter::handleGlobalMetricClassRecorder(OTF2_MetricRef metric,
                                                  OTF2_LocationRef recorder) {

    bool filter_out =
        m_global_MetricClassRecorder_filter.process(metric, recorder);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteMetricClassRecorder(m_def_writer, metric,
                                                      recorder);
    }
}

void TraceWriter::handleGlobalSystemTreeNodeProperty(
    OTF2_SystemTreeNodeRef systemTreeNode, OTF2_StringRef name, OTF2_Type type,
    OTF2_AttributeValue value) {

    bool filter_out = m_global_SystemTreeNodeProperty_filter.process(
        systemTreeNode, name, type, value);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteSystemTreeNodeProperty(
            m_def_writer, systemTreeNode, name, type, value);
    }
}

void TraceWriter::handleGlobalSystemTreeNodeDomain(
    OTF2_SystemTreeNodeRef systemTreeNode,
    OTF2_SystemTreeDomain systemTreeDomain) {

    bool filter_out = m_global_SystemTreeNodeDomain_filter.process(
        systemTreeNode, systemTreeDomain);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteSystemTreeNodeDomain(
            m_def_writer, systemTreeNode, systemTreeDomain);
    }
}

void TraceWriter::handleGlobalLocationGroupProperty(
    OTF2_LocationGroupRef locationGroup, OTF2_StringRef name, OTF2_Type type,
    OTF2_AttributeValue value) {

    bool filter_out = m_global_LocationGroupProperty_filter.process(
        locationGroup, name, type, value);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteLocationGroupProperty(
            m_def_writer, locationGroup, name, type, value);
    }
}

void TraceWriter::handleGlobalLocationProperty(OTF2_LocationRef location,
                                               OTF2_StringRef name,
                                               OTF2_Type type,
                                               OTF2_AttributeValue value) {

    bool filter_out =
        m_global_LocationProperty_filter.process(location, name, type, value);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteLocationProperty(m_def_writer, location, name,
                                                   type, value);
    }
}

void TraceWriter::handleGlobalCartDimension(
    OTF2_CartDimensionRef self, OTF2_StringRef name, uint32_t size,
    OTF2_CartPeriodicity cartPeriodicity) {

    bool filter_out = m_global_CartDimension_filter.process(self, name, size,
                                                            cartPeriodicity);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteCartDimension(m_def_writer, self, name, size,
                                                cartPeriodicity);
    }
}

void TraceWriter::handleGlobalCartTopology(
    OTF2_CartTopologyRef self, OTF2_StringRef name, OTF2_CommRef communicator,
    uint8_t numberOfDimensions, const OTF2_CartDimensionRef *cartDimensions) {

    bool filter_out = m_global_CartTopology_filter.process(
        self, name, communicator, numberOfDimensions, cartDimensions);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteCartTopology(m_def_writer, self, name,
                                               communicator, numberOfDimensions,
                                               cartDimensions);
    }
}

void TraceWriter::handleGlobalCartCoordinate(OTF2_CartTopologyRef cartTopology,
                                             uint32_t rank,
                                             uint8_t numberOfDimensions,
                                             const uint32_t *coordinates) {

    bool filter_out = m_global_CartCoordinate_filter.process(
        cartTopology, rank, numberOfDimensions, coordinates);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteCartCoordinate(
            m_def_writer, cartTopology, rank, numberOfDimensions, coordinates);
    }
}

void TraceWriter::handleGlobalSourceCodeLocation(
    OTF2_SourceCodeLocationRef self, OTF2_StringRef file, uint32_t lineNumber) {

    bool filter_out =
        m_global_SourceCodeLocation_filter.process(self, file, lineNumber);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteSourceCodeLocation(m_def_writer, self, file,
                                                     lineNumber);
    }
}

void TraceWriter::handleGlobalCallingContext(
    OTF2_CallingContextRef self, OTF2_RegionRef region,
    OTF2_SourceCodeLocationRef sourceCodeLocation,
    OTF2_CallingContextRef parent) {

    bool filter_out = m_global_CallingContext_filter.process(
        self, region, sourceCodeLocation, parent);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteCallingContext(m_def_writer, self, region,
                                                 sourceCodeLocation, parent);
    }
}

void TraceWriter::handleGlobalCallingContextProperty(
    OTF2_CallingContextRef callingContext, OTF2_StringRef name, OTF2_Type type,
    OTF2_AttributeValue value) {

    bool filter_out = m_global_CallingContextProperty_filter.process(
        callingContext, name, type, value);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteCallingContextProperty(
            m_def_writer, callingContext, name, type, value);
    }
}

void TraceWriter::handleGlobalInterruptGenerator(
    OTF2_InterruptGeneratorRef self, OTF2_StringRef name,
    OTF2_InterruptGeneratorMode interruptGeneratorMode, OTF2_Base base,
    int64_t exponent, uint64_t period) {

    bool filter_out = m_global_InterruptGenerator_filter.process(
        self, name, interruptGeneratorMode, base, exponent, period);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteInterruptGenerator(m_def_writer, self, name,
                                                     interruptGeneratorMode,
                                                     base, exponent, period);
    }
}

void TraceWriter::handleGlobalIoFileProperty(OTF2_IoFileRef ioFile,
                                             OTF2_StringRef name,
                                             OTF2_Type type,
                                             OTF2_AttributeValue value) {

    bool filter_out =
        m_global_IoFileProperty_filter.process(ioFile, name, type, value);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteIoFileProperty(m_def_writer, ioFile, name,
                                                 type, value);
    }
}

void TraceWriter::handleGlobalIoRegularFile(OTF2_IoFileRef self,
                                            OTF2_StringRef name,
                                            OTF2_SystemTreeNodeRef scope) {

    bool filter_out = m_global_IoRegularFile_filter.process(self, name, scope);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteIoRegularFile(m_def_writer, self, name,
                                                scope);
    }
}

void TraceWriter::handleGlobalIoDirectory(OTF2_IoFileRef self,
                                          OTF2_StringRef name,
                                          OTF2_SystemTreeNodeRef scope) {

    bool filter_out = m_global_IoDirectory_filter.process(self, name, scope);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteIoDirectory(m_def_writer, self, name, scope);
    }
}

void TraceWriter::handleGlobalIoHandle(OTF2_IoHandleRef self,
                                       OTF2_StringRef name, OTF2_IoFileRef file,
                                       OTF2_IoParadigmRef ioParadigm,
                                       OTF2_IoHandleFlag ioHandleFlags,
                                       OTF2_CommRef comm,
                                       OTF2_IoHandleRef parent) {

    bool filter_out = m_global_IoHandle_filter.process(
        self, name, file, ioParadigm, ioHandleFlags, comm, parent);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteIoHandle(m_def_writer, self, name, file,
                                           ioParadigm, ioHandleFlags, comm,
                                           parent);
    }
}

void TraceWriter::handleGlobalIoPreCreatedHandleState(
    OTF2_IoHandleRef ioHandle, OTF2_IoAccessMode mode,
    OTF2_IoStatusFlag statusFlags) {

    bool filter_out = m_global_IoPreCreatedHandleState_filter.process(
        ioHandle, mode, statusFlags);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteIoPreCreatedHandleState(
            m_def_writer, ioHandle, mode, statusFlags);
    }
}

void TraceWriter::handleGlobalCallpathParameter(OTF2_CallpathRef callpath,
                                                OTF2_ParameterRef parameter,
                                                OTF2_Type type,
                                                OTF2_AttributeValue value) {

    bool filter_out = m_global_CallpathParameter_filter.process(
        callpath, parameter, type, value);
    if (!filter_out) {
        OTF2_GlobalDefWriter_WriteCallpathParameter(m_def_writer, callpath,
                                                    parameter, type, value);
    }
}

void TraceWriter::handleLocalMappingTable(OTF2_LocationRef readLocation,
                                          OTF2_MappingType mappingType,
                                          const OTF2_IdMap *idMap) {
    bool filter_out =
        m_local_MappingTable_filter.process(readLocation, mappingType, idMap);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteMappingTable(local_def_writer, mappingType, idMap);
    }
}

void TraceWriter::handleLocalClockOffset(OTF2_LocationRef readLocation,
                                         OTF2_TimeStamp time, int64_t offset,
                                         double standardDeviation) {
    bool filter_out = m_local_ClockOffset_filter.process(
        readLocation, time, offset, standardDeviation);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteClockOffset(local_def_writer, time, offset,
                                        standardDeviation);
    }
}

void TraceWriter::handleLocalString(OTF2_LocationRef readLocation,
                                    OTF2_StringRef self, const char *string) {
    bool filter_out = m_local_String_filter.process(readLocation, self, string);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteString(local_def_writer, self, string);
    }
}

void TraceWriter::handleLocalAttribute(OTF2_LocationRef readLocation,
                                       OTF2_AttributeRef self,
                                       OTF2_StringRef name,
                                       OTF2_StringRef description,
                                       OTF2_Type type) {
    bool filter_out = m_local_Attribute_filter.process(readLocation, self, name,
                                                       description, type);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteAttribute(local_def_writer, self, name, description,
                                      type);
    }
}

void TraceWriter::handleLocalSystemTreeNode(OTF2_LocationRef readLocation,
                                            OTF2_SystemTreeNodeRef self,
                                            OTF2_StringRef name,
                                            OTF2_StringRef className,
                                            OTF2_SystemTreeNodeRef parent) {
    bool filter_out = m_local_SystemTreeNode_filter.process(
        readLocation, self, name, className, parent);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteSystemTreeNode(local_def_writer, self, name,
                                           className, parent);
    }
}

void TraceWriter::handleLocalLocationGroup(
    OTF2_LocationRef readLocation, OTF2_LocationGroupRef self,
    OTF2_StringRef name, OTF2_LocationGroupType locationGroupType,
    OTF2_SystemTreeNodeRef systemTreeParent) {
    bool filter_out = m_local_LocationGroup_filter.process(
        readLocation, self, name, locationGroupType, systemTreeParent);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteLocationGroup(local_def_writer, self, name,
                                          locationGroupType, systemTreeParent);
    }
}

void TraceWriter::handleLocalLocation(OTF2_LocationRef readLocation,
                                      OTF2_LocationRef self,
                                      OTF2_StringRef name,
                                      OTF2_LocationType locationType,
                                      uint64_t numberOfEvents,
                                      OTF2_LocationGroupRef locationGroup) {
    bool filter_out = m_local_Location_filter.process(
        readLocation, self, name, locationType, numberOfEvents, locationGroup);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteLocation(local_def_writer, self, name, locationType,
                                     numberOfEvents, locationGroup);
    }
}

void TraceWriter::handleLocalRegion(
    OTF2_LocationRef readLocation, OTF2_RegionRef self, OTF2_StringRef name,
    OTF2_StringRef canonicalName, OTF2_StringRef description,
    OTF2_RegionRole regionRole, OTF2_Paradigm paradigm,
    OTF2_RegionFlag regionFlags, OTF2_StringRef sourceFile,
    uint32_t beginLineNumber, uint32_t endLineNumber) {
    bool filter_out = m_local_Region_filter.process(
        readLocation, self, name, canonicalName, description, regionRole,
        paradigm, regionFlags, sourceFile, beginLineNumber, endLineNumber);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteRegion(local_def_writer, self, name, canonicalName,
                                   description, regionRole, paradigm,
                                   regionFlags, sourceFile, beginLineNumber,
                                   endLineNumber);
    }
}

void TraceWriter::handleLocalCallsite(OTF2_LocationRef readLocation,
                                      OTF2_CallsiteRef self,
                                      OTF2_StringRef sourceFile,
                                      uint32_t lineNumber,
                                      OTF2_RegionRef enteredRegion,
                                      OTF2_RegionRef leftRegion) {
    bool filter_out = m_local_Callsite_filter.process(
        readLocation, self, sourceFile, lineNumber, enteredRegion, leftRegion);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteCallsite(local_def_writer, self, sourceFile,
                                     lineNumber, enteredRegion, leftRegion);
    }
}

void TraceWriter::handleLocalCallpath(OTF2_LocationRef readLocation,
                                      OTF2_CallpathRef self,
                                      OTF2_CallpathRef parent,
                                      OTF2_RegionRef region) {
    bool filter_out =
        m_local_Callpath_filter.process(readLocation, self, parent, region);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteCallpath(local_def_writer, self, parent, region);
    }
}

void TraceWriter::handleLocalGroup(
    OTF2_LocationRef readLocation, OTF2_GroupRef self, OTF2_StringRef name,
    OTF2_GroupType groupType, OTF2_Paradigm paradigm, OTF2_GroupFlag groupFlags,
    uint32_t numberOfMembers, const uint64_t *members) {
    bool filter_out = m_local_Group_filter.process(
        readLocation, self, name, groupType, paradigm, groupFlags,
        numberOfMembers, members);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteGroup(local_def_writer, self, name, groupType,
                                  paradigm, groupFlags, numberOfMembers,
                                  members);
    }
}

void TraceWriter::handleLocalMetricMember(
    OTF2_LocationRef readLocation, OTF2_MetricMemberRef self,
    OTF2_StringRef name, OTF2_StringRef description, OTF2_MetricType metricType,
    OTF2_MetricMode metricMode, OTF2_Type valueType, OTF2_Base base,
    int64_t exponent, OTF2_StringRef unit) {
    bool filter_out = m_local_MetricMember_filter.process(
        readLocation, self, name, description, metricType, metricMode,
        valueType, base, exponent, unit);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteMetricMember(local_def_writer, self, name,
                                         description, metricType, metricMode,
                                         valueType, base, exponent, unit);
    }
}

void TraceWriter::handleLocalMetricClass(
    OTF2_LocationRef readLocation, OTF2_MetricRef self, uint8_t numberOfMetrics,
    const OTF2_MetricMemberRef *metricMembers,
    OTF2_MetricOccurrence metricOccurrence, OTF2_RecorderKind recorderKind) {
    bool filter_out = m_local_MetricClass_filter.process(
        readLocation, self, numberOfMetrics, metricMembers, metricOccurrence,
        recorderKind);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteMetricClass(local_def_writer, self, numberOfMetrics,
                                        metricMembers, metricOccurrence,
                                        recorderKind);
    }
}

void TraceWriter::handleLocalMetricInstance(OTF2_LocationRef readLocation,
                                            OTF2_MetricRef self,
                                            OTF2_MetricRef metricClass,
                                            OTF2_LocationRef recorder,
                                            OTF2_MetricScope metricScope,
                                            uint64_t scope) {
    bool filter_out = m_local_MetricInstance_filter.process(
        readLocation, self, metricClass, recorder, metricScope, scope);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteMetricInstance(local_def_writer, self, metricClass,
                                           recorder, metricScope, scope);
    }
}

void TraceWriter::handleLocalComm(OTF2_LocationRef readLocation,
                                  OTF2_CommRef self, OTF2_StringRef name,
                                  OTF2_GroupRef group, OTF2_CommRef parent) {
    bool filter_out =
        m_local_Comm_filter.process(readLocation, self, name, group, parent);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteComm(local_def_writer, self, name, group, parent);
    }
}

void TraceWriter::handleLocalParameter(OTF2_LocationRef readLocation,
                                       OTF2_ParameterRef self,
                                       OTF2_StringRef name,
                                       OTF2_ParameterType parameterType) {
    bool filter_out = m_local_Parameter_filter.process(readLocation, self, name,
                                                       parameterType);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteParameter(local_def_writer, self, name,
                                      parameterType);
    }
}

void TraceWriter::handleLocalRmaWin(OTF2_LocationRef readLocation,
                                    OTF2_RmaWinRef self, OTF2_StringRef name,
                                    OTF2_CommRef comm) {
    bool filter_out =
        m_local_RmaWin_filter.process(readLocation, self, name, comm);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteRmaWin(local_def_writer, self, name, comm);
    }
}

void TraceWriter::handleLocalMetricClassRecorder(OTF2_LocationRef readLocation,
                                                 OTF2_MetricRef metric,
                                                 OTF2_LocationRef recorder) {
    bool filter_out = m_local_MetricClassRecorder_filter.process(
        readLocation, metric, recorder);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteMetricClassRecorder(local_def_writer, metric,
                                                recorder);
    }
}

void TraceWriter::handleLocalSystemTreeNodeProperty(
    OTF2_LocationRef readLocation, OTF2_SystemTreeNodeRef systemTreeNode,
    OTF2_StringRef name, OTF2_Type type, OTF2_AttributeValue value) {
    bool filter_out = m_local_SystemTreeNodeProperty_filter.process(
        readLocation, systemTreeNode, name, type, value);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteSystemTreeNodeProperty(
            local_def_writer, systemTreeNode, name, type, value);
    }
}

void TraceWriter::handleLocalSystemTreeNodeDomain(
    OTF2_LocationRef readLocation, OTF2_SystemTreeNodeRef systemTreeNode,
    OTF2_SystemTreeDomain systemTreeDomain) {
    bool filter_out = m_local_SystemTreeNodeDomain_filter.process(
        readLocation, systemTreeNode, systemTreeDomain);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteSystemTreeNodeDomain(
            local_def_writer, systemTreeNode, systemTreeDomain);
    }
}

void TraceWriter::handleLocalLocationGroupProperty(
    OTF2_LocationRef readLocation, OTF2_LocationGroupRef locationGroup,
    OTF2_StringRef name, OTF2_Type type, OTF2_AttributeValue value) {
    bool filter_out = m_local_LocationGroupProperty_filter.process(
        readLocation, locationGroup, name, type, value);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteLocationGroupProperty(
            local_def_writer, locationGroup, name, type, value);
    }
}

void TraceWriter::handleLocalLocationProperty(OTF2_LocationRef readLocation,
                                              OTF2_LocationRef location,
                                              OTF2_StringRef name,
                                              OTF2_Type type,
                                              OTF2_AttributeValue value) {
    bool filter_out = m_local_LocationProperty_filter.process(
        readLocation, location, name, type, value);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteLocationProperty(local_def_writer, location, name,
                                             type, value);
    }
}

void TraceWriter::handleLocalCartDimension(
    OTF2_LocationRef readLocation, OTF2_CartDimensionRef self,
    OTF2_StringRef name, uint32_t size, OTF2_CartPeriodicity cartPeriodicity) {
    bool filter_out = m_local_CartDimension_filter.process(
        readLocation, self, name, size, cartPeriodicity);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteCartDimension(local_def_writer, self, name, size,
                                          cartPeriodicity);
    }
}

void TraceWriter::handleLocalCartTopology(
    OTF2_LocationRef readLocation, OTF2_CartTopologyRef self,
    OTF2_StringRef name, OTF2_CommRef communicator, uint8_t numberOfDimensions,
    const OTF2_CartDimensionRef *cartDimensions) {
    bool filter_out = m_local_CartTopology_filter.process(
        readLocation, self, name, communicator, numberOfDimensions,
        cartDimensions);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteCartTopology(local_def_writer, self, name,
                                         communicator, numberOfDimensions,
                                         cartDimensions);
    }
}

void TraceWriter::handleLocalCartCoordinate(OTF2_LocationRef readLocation,
                                            OTF2_CartTopologyRef cartTopology,
                                            uint32_t rank,
                                            uint8_t numberOfDimensions,
                                            const uint32_t *coordinates) {
    bool filter_out = m_local_CartCoordinate_filter.process(
        readLocation, cartTopology, rank, numberOfDimensions, coordinates);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteCartCoordinate(local_def_writer, cartTopology, rank,
                                           numberOfDimensions, coordinates);
    }
}

void TraceWriter::handleLocalSourceCodeLocation(OTF2_LocationRef readLocation,
                                                OTF2_SourceCodeLocationRef self,
                                                OTF2_StringRef file,
                                                uint32_t lineNumber) {
    bool filter_out = m_local_SourceCodeLocation_filter.process(
        readLocation, self, file, lineNumber);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteSourceCodeLocation(local_def_writer, self, file,
                                               lineNumber);
    }
}

void TraceWriter::handleLocalCallingContext(
    OTF2_LocationRef readLocation, OTF2_CallingContextRef self,
    OTF2_RegionRef region, OTF2_SourceCodeLocationRef sourceCodeLocation,
    OTF2_CallingContextRef parent) {
    bool filter_out = m_local_CallingContext_filter.process(
        readLocation, self, region, sourceCodeLocation, parent);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteCallingContext(local_def_writer, self, region,
                                           sourceCodeLocation, parent);
    }
}

void TraceWriter::handleLocalCallingContextProperty(
    OTF2_LocationRef readLocation, OTF2_CallingContextRef callingContext,
    OTF2_StringRef name, OTF2_Type type, OTF2_AttributeValue value) {
    bool filter_out = m_local_CallingContextProperty_filter.process(
        readLocation, callingContext, name, type, value);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteCallingContextProperty(
            local_def_writer, callingContext, name, type, value);
    }
}

void TraceWriter::handleLocalInterruptGenerator(
    OTF2_LocationRef readLocation, OTF2_InterruptGeneratorRef self,
    OTF2_StringRef name, OTF2_InterruptGeneratorMode interruptGeneratorMode,
    OTF2_Base base, int64_t exponent, uint64_t period) {
    bool filter_out = m_local_InterruptGenerator_filter.process(
        readLocation, self, name, interruptGeneratorMode, base, exponent,
        period);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteInterruptGenerator(local_def_writer, self, name,
                                               interruptGeneratorMode, base,
                                               exponent, period);
    }
}

void TraceWriter::handleLocalIoFileProperty(OTF2_LocationRef readLocation,
                                            OTF2_IoFileRef ioFile,
                                            OTF2_StringRef name, OTF2_Type type,
                                            OTF2_AttributeValue value) {
    bool filter_out = m_local_IoFileProperty_filter.process(
        readLocation, ioFile, name, type, value);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteIoFileProperty(local_def_writer, ioFile, name, type,
                                           value);
    }
}

void TraceWriter::handleLocalIoRegularFile(OTF2_LocationRef readLocation,
                                           OTF2_IoFileRef self,
                                           OTF2_StringRef name,
                                           OTF2_SystemTreeNodeRef scope) {
    bool filter_out =
        m_local_IoRegularFile_filter.process(readLocation, self, name, scope);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteIoRegularFile(local_def_writer, self, name, scope);
    }
}

void TraceWriter::handleLocalIoDirectory(OTF2_LocationRef readLocation,
                                         OTF2_IoFileRef self,
                                         OTF2_StringRef name,
                                         OTF2_SystemTreeNodeRef scope) {
    bool filter_out =
        m_local_IoDirectory_filter.process(readLocation, self, name, scope);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteIoDirectory(local_def_writer, self, name, scope);
    }
}

void TraceWriter::handleLocalIoHandle(OTF2_LocationRef readLocation,
                                      OTF2_IoHandleRef self,
                                      OTF2_StringRef name, OTF2_IoFileRef file,
                                      OTF2_IoParadigmRef ioParadigm,
                                      OTF2_IoHandleFlag ioHandleFlags,
                                      OTF2_CommRef comm,
                                      OTF2_IoHandleRef parent) {
    bool filter_out = m_local_IoHandle_filter.process(
        readLocation, self, name, file, ioParadigm, ioHandleFlags, comm,
        parent);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteIoHandle(local_def_writer, self, name, file,
                                     ioParadigm, ioHandleFlags, comm, parent);
    }
}

void TraceWriter::handleLocalIoPreCreatedHandleState(
    OTF2_LocationRef readLocation, OTF2_IoHandleRef ioHandle,
    OTF2_IoAccessMode mode, OTF2_IoStatusFlag statusFlags) {
    bool filter_out = m_local_IoPreCreatedHandleState_filter.process(
        readLocation, ioHandle, mode, statusFlags);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteIoPreCreatedHandleState(local_def_writer, ioHandle,
                                                    mode, statusFlags);
    }
}

void TraceWriter::handleLocalCallpathParameter(OTF2_LocationRef readLocation,
                                               OTF2_CallpathRef callpath,
                                               OTF2_ParameterRef parameter,
                                               OTF2_Type type,
                                               OTF2_AttributeValue value) {
    bool filter_out = m_local_CallpathParameter_filter.process(
        readLocation, callpath, parameter, type, value);
    if (!filter_out) {
        auto *local_def_writer =
            OTF2_Archive_GetDefWriter(m_archive.get(), readLocation);
        OTF2_DefWriter_WriteCallpathParameter(local_def_writer, callpath,
                                              parameter, type, value);
    }
}

void TraceWriter::handleBufferFlushEvent(OTF2_LocationRef location,
                                         OTF2_TimeStamp time,
                                         OTF2_AttributeList *attributes,
                                         OTF2_TimeStamp stopTime) {
    bool filter_out = m_event_BufferFlush_filter.process(location, time,
                                                         attributes, stopTime);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_BufferFlush(event_writer, attributes, time, stopTime);
    }
}

void TraceWriter::handleMeasurementOnOffEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_MeasurementMode measurementMode) {
    bool filter_out = m_event_MeasurementOnOff_filter.process(
        location, time, attributes, measurementMode);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MeasurementOnOff(event_writer, attributes, time,
                                        measurementMode);
    }
}

void TraceWriter::handleEnterEvent(OTF2_LocationRef location,
                                   OTF2_TimeStamp time,
                                   OTF2_AttributeList *attributes,
                                   OTF2_RegionRef region) {
    bool filter_out =
        m_event_Enter_filter.process(location, time, attributes, region);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_Enter(event_writer, attributes, time, region);
    }
}

void TraceWriter::handleLeaveEvent(OTF2_LocationRef location,
                                   OTF2_TimeStamp time,
                                   OTF2_AttributeList *attributes,
                                   OTF2_RegionRef region) {
    bool filter_out =
        m_event_Leave_filter.process(location, time, attributes, region);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_Leave(event_writer, attributes, time, region);
    }
}

void TraceWriter::handleMpiSendEvent(OTF2_LocationRef location,
                                     OTF2_TimeStamp time,
                                     OTF2_AttributeList *attributes,
                                     uint32_t receiver,
                                     OTF2_CommRef communicator, uint32_t msgTag,
                                     uint64_t msgLength) {
    bool filter_out = m_event_MpiSend_filter.process(
        location, time, attributes, receiver, communicator, msgTag, msgLength);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiSend(event_writer, attributes, time, receiver,
                               communicator, msgTag, msgLength);
    }
}

void TraceWriter::handleMpiIsendEvent(OTF2_LocationRef location,
                                      OTF2_TimeStamp time,
                                      OTF2_AttributeList *attributes,
                                      uint32_t receiver,
                                      OTF2_CommRef communicator,
                                      uint32_t msgTag, uint64_t msgLength,
                                      uint64_t requestID) {
    bool filter_out = m_event_MpiIsend_filter.process(
        location, time, attributes, receiver, communicator, msgTag, msgLength,
        requestID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiIsend(event_writer, attributes, time, receiver,
                                communicator, msgTag, msgLength, requestID);
    }
}

void TraceWriter::handleMpiIsendCompleteEvent(OTF2_LocationRef location,
                                              OTF2_TimeStamp time,
                                              OTF2_AttributeList *attributes,
                                              uint64_t requestID) {
    bool filter_out = m_event_MpiIsendComplete_filter.process(
        location, time, attributes, requestID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiIsendComplete(event_writer, attributes, time,
                                        requestID);
    }
}

void TraceWriter::handleMpiIrecvRequestEvent(OTF2_LocationRef location,
                                             OTF2_TimeStamp time,
                                             OTF2_AttributeList *attributes,
                                             uint64_t requestID) {
    bool filter_out = m_event_MpiIrecvRequest_filter.process(
        location, time, attributes, requestID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiIrecvRequest(event_writer, attributes, time,
                                       requestID);
    }
}

void TraceWriter::handleMpiRecvEvent(OTF2_LocationRef location,
                                     OTF2_TimeStamp time,
                                     OTF2_AttributeList *attributes,
                                     uint32_t sender, OTF2_CommRef communicator,
                                     uint32_t msgTag, uint64_t msgLength) {
    bool filter_out = m_event_MpiRecv_filter.process(
        location, time, attributes, sender, communicator, msgTag, msgLength);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiRecv(event_writer, attributes, time, sender,
                               communicator, msgTag, msgLength);
    }
}

void TraceWriter::handleMpiIrecvEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, uint32_t sender, OTF2_CommRef communicator,
    uint32_t msgTag, uint64_t msgLength, uint64_t requestID) {
    bool filter_out = m_event_MpiIrecv_filter.process(
        location, time, attributes, sender, communicator, msgTag, msgLength,
        requestID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiIrecv(event_writer, attributes, time, sender,
                                communicator, msgTag, msgLength, requestID);
    }
}

void TraceWriter::handleMpiRequestTestEvent(OTF2_LocationRef location,
                                            OTF2_TimeStamp time,
                                            OTF2_AttributeList *attributes,
                                            uint64_t requestID) {
    bool filter_out = m_event_MpiRequestTest_filter.process(
        location, time, attributes, requestID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiRequestTest(event_writer, attributes, time,
                                      requestID);
    }
}

void TraceWriter::handleMpiRequestCancelledEvent(OTF2_LocationRef location,
                                                 OTF2_TimeStamp time,
                                                 OTF2_AttributeList *attributes,
                                                 uint64_t requestID) {
    bool filter_out = m_event_MpiRequestCancelled_filter.process(
        location, time, attributes, requestID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiRequestCancelled(event_writer, attributes, time,
                                           requestID);
    }
}

void TraceWriter::handleMpiCollectiveBeginEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes) {
    bool filter_out =
        m_event_MpiCollectiveBegin_filter.process(location, time, attributes);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiCollectiveBegin(event_writer, attributes, time);
    }
}

void TraceWriter::handleMpiCollectiveEndEvent(OTF2_LocationRef location,
                                              OTF2_TimeStamp time,
                                              OTF2_AttributeList *attributes,
                                              OTF2_CollectiveOp collectiveOp,
                                              OTF2_CommRef communicator,
                                              uint32_t root, uint64_t sizeSent,
                                              uint64_t sizeReceived) {
    bool filter_out = m_event_MpiCollectiveEnd_filter.process(
        location, time, attributes, collectiveOp, communicator, root, sizeSent,
        sizeReceived);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_MpiCollectiveEnd(event_writer, attributes, time,
                                        collectiveOp, communicator, root,
                                        sizeSent, sizeReceived);
    }
}

void TraceWriter::handleOmpForkEvent(OTF2_LocationRef location,
                                     OTF2_TimeStamp time,
                                     OTF2_AttributeList *attributes,
                                     uint32_t numberOfRequestedThreads) {
    bool filter_out = m_event_OmpFork_filter.process(location, time, attributes,
                                                     numberOfRequestedThreads);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_OmpFork(event_writer, attributes, time,
                               numberOfRequestedThreads);
    }
}

void TraceWriter::handleOmpJoinEvent(OTF2_LocationRef location,
                                     OTF2_TimeStamp time,
                                     OTF2_AttributeList *attributes) {
    bool filter_out =
        m_event_OmpJoin_filter.process(location, time, attributes);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_OmpJoin(event_writer, attributes, time);
    }
}

void TraceWriter::handleOmpAcquireLockEvent(OTF2_LocationRef location,
                                            OTF2_TimeStamp time,
                                            OTF2_AttributeList *attributes,
                                            uint32_t lockID,
                                            uint32_t acquisitionOrder) {
    bool filter_out = m_event_OmpAcquireLock_filter.process(
        location, time, attributes, lockID, acquisitionOrder);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_OmpAcquireLock(event_writer, attributes, time, lockID,
                                      acquisitionOrder);
    }
}

void TraceWriter::handleOmpReleaseLockEvent(OTF2_LocationRef location,
                                            OTF2_TimeStamp time,
                                            OTF2_AttributeList *attributes,
                                            uint32_t lockID,
                                            uint32_t acquisitionOrder) {
    bool filter_out = m_event_OmpReleaseLock_filter.process(
        location, time, attributes, lockID, acquisitionOrder);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_OmpReleaseLock(event_writer, attributes, time, lockID,
                                      acquisitionOrder);
    }
}

void TraceWriter::handleOmpTaskCreateEvent(OTF2_LocationRef location,
                                           OTF2_TimeStamp time,
                                           OTF2_AttributeList *attributes,
                                           uint64_t taskID) {
    bool filter_out = m_event_OmpTaskCreate_filter.process(location, time,
                                                           attributes, taskID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_OmpTaskCreate(event_writer, attributes, time, taskID);
    }
}

void TraceWriter::handleOmpTaskSwitchEvent(OTF2_LocationRef location,
                                           OTF2_TimeStamp time,
                                           OTF2_AttributeList *attributes,
                                           uint64_t taskID) {
    bool filter_out = m_event_OmpTaskSwitch_filter.process(location, time,
                                                           attributes, taskID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_OmpTaskSwitch(event_writer, attributes, time, taskID);
    }
}

void TraceWriter::handleOmpTaskCompleteEvent(OTF2_LocationRef location,
                                             OTF2_TimeStamp time,
                                             OTF2_AttributeList *attributes,
                                             uint64_t taskID) {
    bool filter_out = m_event_OmpTaskComplete_filter.process(
        location, time, attributes, taskID);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_OmpTaskComplete(event_writer, attributes, time, taskID);
    }
}

void TraceWriter::handleMetricEvent(OTF2_LocationRef location,
                                    OTF2_TimeStamp time,
                                    OTF2_AttributeList *attributes,
                                    OTF2_MetricRef metric,
                                    uint8_t numberOfMetrics,
                                    const OTF2_Type *typeIDs,
                                    const OTF2_MetricValue *metricValues) {
    bool filter_out =
        m_event_Metric_filter.process(location, time, attributes, metric,
                                      numberOfMetrics, typeIDs, metricValues);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_Metric(event_writer, attributes, time, metric,
                              numberOfMetrics, typeIDs, metricValues);
    }
}

void TraceWriter::handleParameterStringEvent(OTF2_LocationRef location,
                                             OTF2_TimeStamp time,
                                             OTF2_AttributeList *attributes,
                                             OTF2_ParameterRef parameter,
                                             OTF2_StringRef string) {
    bool filter_out = m_event_ParameterString_filter.process(
        location, time, attributes, parameter, string);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ParameterString(event_writer, attributes, time,
                                       parameter, string);
    }
}

void TraceWriter::handleParameterIntEvent(OTF2_LocationRef location,
                                          OTF2_TimeStamp time,
                                          OTF2_AttributeList *attributes,
                                          OTF2_ParameterRef parameter,
                                          int64_t value) {
    bool filter_out = m_event_ParameterInt_filter.process(
        location, time, attributes, parameter, value);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ParameterInt(event_writer, attributes, time, parameter,
                                    value);
    }
}

void TraceWriter::handleParameterUnsignedIntEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_ParameterRef parameter,
    uint64_t value) {
    bool filter_out = m_event_ParameterUnsignedInt_filter.process(
        location, time, attributes, parameter, value);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ParameterUnsignedInt(event_writer, attributes, time,
                                            parameter, value);
    }
}

void TraceWriter::handleRmaWinCreateEvent(OTF2_LocationRef location,
                                          OTF2_TimeStamp time,
                                          OTF2_AttributeList *attributes,
                                          OTF2_RmaWinRef win) {
    bool filter_out =
        m_event_RmaWinCreate_filter.process(location, time, attributes, win);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaWinCreate(event_writer, attributes, time, win);
    }
}

void TraceWriter::handleRmaWinDestroyEvent(OTF2_LocationRef location,
                                           OTF2_TimeStamp time,
                                           OTF2_AttributeList *attributes,
                                           OTF2_RmaWinRef win) {
    bool filter_out =
        m_event_RmaWinDestroy_filter.process(location, time, attributes, win);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaWinDestroy(event_writer, attributes, time, win);
    }
}

void TraceWriter::handleRmaCollectiveBeginEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes) {
    bool filter_out =
        m_event_RmaCollectiveBegin_filter.process(location, time, attributes);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaCollectiveBegin(event_writer, attributes, time);
    }
}

void TraceWriter::handleRmaCollectiveEndEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_CollectiveOp collectiveOp,
    OTF2_RmaSyncLevel syncLevel, OTF2_RmaWinRef win, uint32_t root,
    uint64_t bytesSent, uint64_t bytesReceived) {
    bool filter_out = m_event_RmaCollectiveEnd_filter.process(
        location, time, attributes, collectiveOp, syncLevel, win, root,
        bytesSent, bytesReceived);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaCollectiveEnd(event_writer, attributes, time,
                                        collectiveOp, syncLevel, win, root,
                                        bytesSent, bytesReceived);
    }
}

void TraceWriter::handleRmaGroupSyncEvent(OTF2_LocationRef location,
                                          OTF2_TimeStamp time,
                                          OTF2_AttributeList *attributes,
                                          OTF2_RmaSyncLevel syncLevel,
                                          OTF2_RmaWinRef win,
                                          OTF2_GroupRef group) {
    bool filter_out = m_event_RmaGroupSync_filter.process(
        location, time, attributes, syncLevel, win, group);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaGroupSync(event_writer, attributes, time, syncLevel,
                                    win, group);
    }
}

void TraceWriter::handleRmaRequestLockEvent(OTF2_LocationRef location,
                                            OTF2_TimeStamp time,
                                            OTF2_AttributeList *attributes,
                                            OTF2_RmaWinRef win, uint32_t remote,
                                            uint64_t lockId,
                                            OTF2_LockType lockType) {
    bool filter_out = m_event_RmaRequestLock_filter.process(
        location, time, attributes, win, remote, lockId, lockType);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaRequestLock(event_writer, attributes, time, win,
                                      remote, lockId, lockType);
    }
}

void TraceWriter::handleRmaAcquireLockEvent(OTF2_LocationRef location,
                                            OTF2_TimeStamp time,
                                            OTF2_AttributeList *attributes,
                                            OTF2_RmaWinRef win, uint32_t remote,
                                            uint64_t lockId,
                                            OTF2_LockType lockType) {
    bool filter_out = m_event_RmaAcquireLock_filter.process(
        location, time, attributes, win, remote, lockId, lockType);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaAcquireLock(event_writer, attributes, time, win,
                                      remote, lockId, lockType);
    }
}

void TraceWriter::handleRmaTryLockEvent(OTF2_LocationRef location,
                                        OTF2_TimeStamp time,
                                        OTF2_AttributeList *attributes,
                                        OTF2_RmaWinRef win, uint32_t remote,
                                        uint64_t lockId,
                                        OTF2_LockType lockType) {
    bool filter_out = m_event_RmaTryLock_filter.process(
        location, time, attributes, win, remote, lockId, lockType);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaTryLock(event_writer, attributes, time, win, remote,
                                  lockId, lockType);
    }
}

void TraceWriter::handleRmaReleaseLockEvent(OTF2_LocationRef location,
                                            OTF2_TimeStamp time,
                                            OTF2_AttributeList *attributes,
                                            OTF2_RmaWinRef win, uint32_t remote,
                                            uint64_t lockId) {
    bool filter_out = m_event_RmaReleaseLock_filter.process(
        location, time, attributes, win, remote, lockId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaReleaseLock(event_writer, attributes, time, win,
                                      remote, lockId);
    }
}

void TraceWriter::handleRmaSyncEvent(OTF2_LocationRef location,
                                     OTF2_TimeStamp time,
                                     OTF2_AttributeList *attributes,
                                     OTF2_RmaWinRef win, uint32_t remote,
                                     OTF2_RmaSyncType syncType) {
    bool filter_out = m_event_RmaSync_filter.process(location, time, attributes,
                                                     win, remote, syncType);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaSync(event_writer, attributes, time, win, remote,
                               syncType);
    }
}

void TraceWriter::handleRmaWaitChangeEvent(OTF2_LocationRef location,
                                           OTF2_TimeStamp time,
                                           OTF2_AttributeList *attributes,
                                           OTF2_RmaWinRef win) {
    bool filter_out =
        m_event_RmaWaitChange_filter.process(location, time, attributes, win);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaWaitChange(event_writer, attributes, time, win);
    }
}

void TraceWriter::handleRmaPutEvent(OTF2_LocationRef location,
                                    OTF2_TimeStamp time,
                                    OTF2_AttributeList *attributes,
                                    OTF2_RmaWinRef win, uint32_t remote,
                                    uint64_t bytes, uint64_t matchingId) {
    bool filter_out = m_event_RmaPut_filter.process(
        location, time, attributes, win, remote, bytes, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaPut(event_writer, attributes, time, win, remote,
                              bytes, matchingId);
    }
}

void TraceWriter::handleRmaGetEvent(OTF2_LocationRef location,
                                    OTF2_TimeStamp time,
                                    OTF2_AttributeList *attributes,
                                    OTF2_RmaWinRef win, uint32_t remote,
                                    uint64_t bytes, uint64_t matchingId) {
    bool filter_out = m_event_RmaGet_filter.process(
        location, time, attributes, win, remote, bytes, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaGet(event_writer, attributes, time, win, remote,
                              bytes, matchingId);
    }
}

void TraceWriter::handleRmaAtomicEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_RmaWinRef win, uint32_t remote,
    OTF2_RmaAtomicType type, uint64_t bytesSent, uint64_t bytesReceived,
    uint64_t matchingId) {
    bool filter_out = m_event_RmaAtomic_filter.process(
        location, time, attributes, win, remote, type, bytesSent, bytesReceived,
        matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaAtomic(event_writer, attributes, time, win, remote,
                                 type, bytesSent, bytesReceived, matchingId);
    }
}

void TraceWriter::handleRmaOpCompleteBlockingEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_RmaWinRef win, uint64_t matchingId) {
    bool filter_out = m_event_RmaOpCompleteBlocking_filter.process(
        location, time, attributes, win, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaOpCompleteBlocking(event_writer, attributes, time,
                                             win, matchingId);
    }
}

void TraceWriter::handleRmaOpCompleteNonBlockingEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_RmaWinRef win, uint64_t matchingId) {
    bool filter_out = m_event_RmaOpCompleteNonBlocking_filter.process(
        location, time, attributes, win, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaOpCompleteNonBlocking(event_writer, attributes, time,
                                                win, matchingId);
    }
}

void TraceWriter::handleRmaOpTestEvent(OTF2_LocationRef location,
                                       OTF2_TimeStamp time,
                                       OTF2_AttributeList *attributes,
                                       OTF2_RmaWinRef win,
                                       uint64_t matchingId) {
    bool filter_out = m_event_RmaOpTest_filter.process(
        location, time, attributes, win, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaOpTest(event_writer, attributes, time, win,
                                 matchingId);
    }
}

void TraceWriter::handleRmaOpCompleteRemoteEvent(OTF2_LocationRef location,
                                                 OTF2_TimeStamp time,
                                                 OTF2_AttributeList *attributes,
                                                 OTF2_RmaWinRef win,
                                                 uint64_t matchingId) {
    bool filter_out = m_event_RmaOpCompleteRemote_filter.process(
        location, time, attributes, win, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_RmaOpCompleteRemote(event_writer, attributes, time, win,
                                           matchingId);
    }
}

void TraceWriter::handleThreadForkEvent(OTF2_LocationRef location,
                                        OTF2_TimeStamp time,
                                        OTF2_AttributeList *attributes,
                                        OTF2_Paradigm model,
                                        uint32_t numberOfRequestedThreads) {
    bool filter_out = m_event_ThreadFork_filter.process(
        location, time, attributes, model, numberOfRequestedThreads);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadFork(event_writer, attributes, time, model,
                                  numberOfRequestedThreads);
    }
}

void TraceWriter::handleThreadJoinEvent(OTF2_LocationRef location,
                                        OTF2_TimeStamp time,
                                        OTF2_AttributeList *attributes,
                                        OTF2_Paradigm model) {
    bool filter_out =
        m_event_ThreadJoin_filter.process(location, time, attributes, model);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadJoin(event_writer, attributes, time, model);
    }
}

void TraceWriter::handleThreadTeamBeginEvent(OTF2_LocationRef location,
                                             OTF2_TimeStamp time,
                                             OTF2_AttributeList *attributes,
                                             OTF2_CommRef threadTeam) {
    bool filter_out = m_event_ThreadTeamBegin_filter.process(
        location, time, attributes, threadTeam);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadTeamBegin(event_writer, attributes, time,
                                       threadTeam);
    }
}

void TraceWriter::handleThreadTeamEndEvent(OTF2_LocationRef location,
                                           OTF2_TimeStamp time,
                                           OTF2_AttributeList *attributes,
                                           OTF2_CommRef threadTeam) {
    bool filter_out = m_event_ThreadTeamEnd_filter.process(
        location, time, attributes, threadTeam);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadTeamEnd(event_writer, attributes, time,
                                     threadTeam);
    }
}

void TraceWriter::handleThreadAcquireLockEvent(OTF2_LocationRef location,
                                               OTF2_TimeStamp time,
                                               OTF2_AttributeList *attributes,
                                               OTF2_Paradigm model,
                                               uint32_t lockID,
                                               uint32_t acquisitionOrder) {
    bool filter_out = m_event_ThreadAcquireLock_filter.process(
        location, time, attributes, model, lockID, acquisitionOrder);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadAcquireLock(event_writer, attributes, time, model,
                                         lockID, acquisitionOrder);
    }
}

void TraceWriter::handleThreadReleaseLockEvent(OTF2_LocationRef location,
                                               OTF2_TimeStamp time,
                                               OTF2_AttributeList *attributes,
                                               OTF2_Paradigm model,
                                               uint32_t lockID,
                                               uint32_t acquisitionOrder) {
    bool filter_out = m_event_ThreadReleaseLock_filter.process(
        location, time, attributes, model, lockID, acquisitionOrder);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadReleaseLock(event_writer, attributes, time, model,
                                         lockID, acquisitionOrder);
    }
}

void TraceWriter::handleThreadTaskCreateEvent(OTF2_LocationRef location,
                                              OTF2_TimeStamp time,
                                              OTF2_AttributeList *attributes,
                                              OTF2_CommRef threadTeam,
                                              uint32_t creatingThread,
                                              uint32_t generationNumber) {
    bool filter_out = m_event_ThreadTaskCreate_filter.process(
        location, time, attributes, threadTeam, creatingThread,
        generationNumber);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadTaskCreate(event_writer, attributes, time,
                                        threadTeam, creatingThread,
                                        generationNumber);
    }
}

void TraceWriter::handleThreadTaskSwitchEvent(OTF2_LocationRef location,
                                              OTF2_TimeStamp time,
                                              OTF2_AttributeList *attributes,
                                              OTF2_CommRef threadTeam,
                                              uint32_t creatingThread,
                                              uint32_t generationNumber) {
    bool filter_out = m_event_ThreadTaskSwitch_filter.process(
        location, time, attributes, threadTeam, creatingThread,
        generationNumber);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadTaskSwitch(event_writer, attributes, time,
                                        threadTeam, creatingThread,
                                        generationNumber);
    }
}

void TraceWriter::handleThreadTaskCompleteEvent(OTF2_LocationRef location,
                                                OTF2_TimeStamp time,
                                                OTF2_AttributeList *attributes,
                                                OTF2_CommRef threadTeam,
                                                uint32_t creatingThread,
                                                uint32_t generationNumber) {
    bool filter_out = m_event_ThreadTaskComplete_filter.process(
        location, time, attributes, threadTeam, creatingThread,
        generationNumber);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadTaskComplete(event_writer, attributes, time,
                                          threadTeam, creatingThread,
                                          generationNumber);
    }
}

void TraceWriter::handleThreadCreateEvent(OTF2_LocationRef location,
                                          OTF2_TimeStamp time,
                                          OTF2_AttributeList *attributes,
                                          OTF2_CommRef threadContingent,
                                          uint64_t sequenceCount) {
    bool filter_out = m_event_ThreadCreate_filter.process(
        location, time, attributes, threadContingent, sequenceCount);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadCreate(event_writer, attributes, time,
                                    threadContingent, sequenceCount);
    }
}

void TraceWriter::handleThreadBeginEvent(OTF2_LocationRef location,
                                         OTF2_TimeStamp time,
                                         OTF2_AttributeList *attributes,
                                         OTF2_CommRef threadContingent,
                                         uint64_t sequenceCount) {
    bool filter_out = m_event_ThreadBegin_filter.process(
        location, time, attributes, threadContingent, sequenceCount);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadBegin(event_writer, attributes, time,
                                   threadContingent, sequenceCount);
    }
}

void TraceWriter::handleThreadWaitEvent(OTF2_LocationRef location,
                                        OTF2_TimeStamp time,
                                        OTF2_AttributeList *attributes,
                                        OTF2_CommRef threadContingent,
                                        uint64_t sequenceCount) {
    bool filter_out = m_event_ThreadWait_filter.process(
        location, time, attributes, threadContingent, sequenceCount);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadWait(event_writer, attributes, time,
                                  threadContingent, sequenceCount);
    }
}

void TraceWriter::handleThreadEndEvent(OTF2_LocationRef location,
                                       OTF2_TimeStamp time,
                                       OTF2_AttributeList *attributes,
                                       OTF2_CommRef threadContingent,
                                       uint64_t sequenceCount) {
    bool filter_out = m_event_ThreadEnd_filter.process(
        location, time, attributes, threadContingent, sequenceCount);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ThreadEnd(event_writer, attributes, time,
                                 threadContingent, sequenceCount);
    }
}

void TraceWriter::handleCallingContextEnterEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_CallingContextRef callingContext,
    uint32_t unwindDistance) {
    bool filter_out = m_event_CallingContextEnter_filter.process(
        location, time, attributes, callingContext, unwindDistance);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_CallingContextEnter(event_writer, attributes, time,
                                           callingContext, unwindDistance);
    }
}

void TraceWriter::handleCallingContextLeaveEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_CallingContextRef callingContext) {
    bool filter_out = m_event_CallingContextLeave_filter.process(
        location, time, attributes, callingContext);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_CallingContextLeave(event_writer, attributes, time,
                                           callingContext);
    }
}

void TraceWriter::handleCallingContextSampleEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_CallingContextRef callingContext,
    uint32_t unwindDistance, OTF2_InterruptGeneratorRef interruptGenerator) {
    bool filter_out = m_event_CallingContextSample_filter.process(
        location, time, attributes, callingContext, unwindDistance,
        interruptGenerator);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_CallingContextSample(event_writer, attributes, time,
                                            callingContext, unwindDistance,
                                            interruptGenerator);
    }
}

void TraceWriter::handleIoCreateHandleEvent(OTF2_LocationRef location,
                                            OTF2_TimeStamp time,
                                            OTF2_AttributeList *attributes,
                                            OTF2_IoHandleRef handle,
                                            OTF2_IoAccessMode mode,
                                            OTF2_IoCreationFlag creationFlags,
                                            OTF2_IoStatusFlag statusFlags) {
    bool filter_out = m_event_IoCreateHandle_filter.process(
        location, time, attributes, handle, mode, creationFlags, statusFlags);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoCreateHandle(event_writer, attributes, time, handle,
                                      mode, creationFlags, statusFlags);
    }
}

void TraceWriter::handleIoDestroyHandleEvent(OTF2_LocationRef location,
                                             OTF2_TimeStamp time,
                                             OTF2_AttributeList *attributes,
                                             OTF2_IoHandleRef handle) {
    bool filter_out = m_event_IoDestroyHandle_filter.process(
        location, time, attributes, handle);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoDestroyHandle(event_writer, attributes, time, handle);
    }
}

void TraceWriter::handleIoDuplicateHandleEvent(OTF2_LocationRef location,
                                               OTF2_TimeStamp time,
                                               OTF2_AttributeList *attributes,
                                               OTF2_IoHandleRef oldHandle,
                                               OTF2_IoHandleRef newHandle,
                                               OTF2_IoStatusFlag statusFlags) {
    bool filter_out = m_event_IoDuplicateHandle_filter.process(
        location, time, attributes, oldHandle, newHandle, statusFlags);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoDuplicateHandle(event_writer, attributes, time,
                                         oldHandle, newHandle, statusFlags);
    }
}

void TraceWriter::handleIoSeekEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_IoHandleRef handle,
    int64_t offsetRequest, OTF2_IoSeekOption whence, uint64_t offsetResult) {
    bool filter_out =
        m_event_IoSeek_filter.process(location, time, attributes, handle,
                                      offsetRequest, whence, offsetResult);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoSeek(event_writer, attributes, time, handle,
                              offsetRequest, whence, offsetResult);
    }
}

void TraceWriter::handleIoChangeStatusFlagsEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_IoHandleRef handle,
    OTF2_IoStatusFlag statusFlags) {
    bool filter_out = m_event_IoChangeStatusFlags_filter.process(
        location, time, attributes, handle, statusFlags);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoChangeStatusFlags(event_writer, attributes, time,
                                           handle, statusFlags);
    }
}

void TraceWriter::handleIoDeleteFileEvent(OTF2_LocationRef location,
                                          OTF2_TimeStamp time,
                                          OTF2_AttributeList *attributes,
                                          OTF2_IoParadigmRef ioParadigm,
                                          OTF2_IoFileRef file) {
    bool filter_out = m_event_IoDeleteFile_filter.process(
        location, time, attributes, ioParadigm, file);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoDeleteFile(event_writer, attributes, time, ioParadigm,
                                    file);
    }
}

void TraceWriter::handleIoOperationBeginEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_IoHandleRef handle,
    OTF2_IoOperationMode mode, OTF2_IoOperationFlag operationFlags,
    uint64_t bytesRequest, uint64_t matchingId) {
    bool filter_out = m_event_IoOperationBegin_filter.process(
        location, time, attributes, handle, mode, operationFlags, bytesRequest,
        matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoOperationBegin(event_writer, attributes, time, handle,
                                        mode, operationFlags, bytesRequest,
                                        matchingId);
    }
}

void TraceWriter::handleIoOperationTestEvent(OTF2_LocationRef location,
                                             OTF2_TimeStamp time,
                                             OTF2_AttributeList *attributes,
                                             OTF2_IoHandleRef handle,
                                             uint64_t matchingId) {
    bool filter_out = m_event_IoOperationTest_filter.process(
        location, time, attributes, handle, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoOperationTest(event_writer, attributes, time, handle,
                                       matchingId);
    }
}

void TraceWriter::handleIoOperationIssuedEvent(OTF2_LocationRef location,
                                               OTF2_TimeStamp time,
                                               OTF2_AttributeList *attributes,
                                               OTF2_IoHandleRef handle,
                                               uint64_t matchingId) {
    bool filter_out = m_event_IoOperationIssued_filter.process(
        location, time, attributes, handle, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoOperationIssued(event_writer, attributes, time, handle,
                                         matchingId);
    }
}

void TraceWriter::handleIoOperationCompleteEvent(OTF2_LocationRef location,
                                                 OTF2_TimeStamp time,
                                                 OTF2_AttributeList *attributes,
                                                 OTF2_IoHandleRef handle,
                                                 uint64_t bytesResult,
                                                 uint64_t matchingId) {
    bool filter_out = m_event_IoOperationComplete_filter.process(
        location, time, attributes, handle, bytesResult, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoOperationComplete(event_writer, attributes, time,
                                           handle, bytesResult, matchingId);
    }
}

void TraceWriter::handleIoOperationCancelledEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_IoHandleRef handle,
    uint64_t matchingId) {
    bool filter_out = m_event_IoOperationCancelled_filter.process(
        location, time, attributes, handle, matchingId);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoOperationCancelled(event_writer, attributes, time,
                                            handle, matchingId);
    }
}

void TraceWriter::handleIoAcquireLockEvent(OTF2_LocationRef location,
                                           OTF2_TimeStamp time,
                                           OTF2_AttributeList *attributes,
                                           OTF2_IoHandleRef handle,
                                           OTF2_LockType lockType) {
    bool filter_out = m_event_IoAcquireLock_filter.process(
        location, time, attributes, handle, lockType);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoAcquireLock(event_writer, attributes, time, handle,
                                     lockType);
    }
}

void TraceWriter::handleIoReleaseLockEvent(OTF2_LocationRef location,
                                           OTF2_TimeStamp time,
                                           OTF2_AttributeList *attributes,
                                           OTF2_IoHandleRef handle,
                                           OTF2_LockType lockType) {
    bool filter_out = m_event_IoReleaseLock_filter.process(
        location, time, attributes, handle, lockType);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoReleaseLock(event_writer, attributes, time, handle,
                                     lockType);
    }
}

void TraceWriter::handleIoTryLockEvent(OTF2_LocationRef location,
                                       OTF2_TimeStamp time,
                                       OTF2_AttributeList *attributes,
                                       OTF2_IoHandleRef handle,
                                       OTF2_LockType lockType) {
    bool filter_out = m_event_IoTryLock_filter.process(
        location, time, attributes, handle, lockType);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_IoTryLock(event_writer, attributes, time, handle,
                                 lockType);
    }
}

void TraceWriter::handleProgramBeginEvent(
    OTF2_LocationRef location, OTF2_TimeStamp time,
    OTF2_AttributeList *attributes, OTF2_StringRef programName,
    uint32_t numberOfArguments, const OTF2_StringRef *programArguments) {
    bool filter_out = m_event_ProgramBegin_filter.process(
        location, time, attributes, programName, numberOfArguments,
        programArguments);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ProgramBegin(event_writer, attributes, time, programName,
                                    numberOfArguments, programArguments);
    }
}

void TraceWriter::handleProgramEndEvent(OTF2_LocationRef location,
                                        OTF2_TimeStamp time,
                                        OTF2_AttributeList *attributes,
                                        int64_t exitStatus) {
    bool filter_out = m_event_ProgramEnd_filter.process(location, time,
                                                        attributes, exitStatus);
    if (!filter_out) {
        auto *event_writer =
            OTF2_Archive_GetEvtWriter(m_archive.get(), location);
        OTF2_EvtWriter_ProgramEnd(event_writer, attributes, time, exitStatus);
    }
}

void TraceWriter::registerClockPropertiesGlobalFilter(
    GlobalClockPropertiesFilter filter) {
    m_global_ClockProperties_filter.add(filter);
}

void TraceWriter::registerParadigmGlobalFilter(GlobalParadigmFilter filter) {
    m_global_Paradigm_filter.add(filter);
}

void TraceWriter::registerParadigmPropertyGlobalFilter(
    GlobalParadigmPropertyFilter filter) {
    m_global_ParadigmProperty_filter.add(filter);
}

void TraceWriter::registerIoParadigmGlobalFilter(
    GlobalIoParadigmFilter filter) {
    m_global_IoParadigm_filter.add(filter);
}

void TraceWriter::registerStringGlobalFilter(GlobalStringFilter filter) {
    m_global_String_filter.add(filter);
}

void TraceWriter::registerAttributeGlobalFilter(GlobalAttributeFilter filter) {
    m_global_Attribute_filter.add(filter);
}

void TraceWriter::registerSystemTreeNodeGlobalFilter(
    GlobalSystemTreeNodeFilter filter) {
    m_global_SystemTreeNode_filter.add(filter);
}

void TraceWriter::registerLocationGroupGlobalFilter(
    GlobalLocationGroupFilter filter) {
    m_global_LocationGroup_filter.add(filter);
}

void TraceWriter::registerLocationGlobalFilter(GlobalLocationFilter filter) {
    m_global_Location_filter.add(filter);
}

void TraceWriter::registerRegionGlobalFilter(GlobalRegionFilter filter) {
    m_global_Region_filter.add(filter);
}

void TraceWriter::registerCallsiteGlobalFilter(GlobalCallsiteFilter filter) {
    m_global_Callsite_filter.add(filter);
}

void TraceWriter::registerCallpathGlobalFilter(GlobalCallpathFilter filter) {
    m_global_Callpath_filter.add(filter);
}

void TraceWriter::registerGroupGlobalFilter(GlobalGroupFilter filter) {
    m_global_Group_filter.add(filter);
}

void TraceWriter::registerMetricMemberGlobalFilter(
    GlobalMetricMemberFilter filter) {
    m_global_MetricMember_filter.add(filter);
}

void TraceWriter::registerMetricClassGlobalFilter(
    GlobalMetricClassFilter filter) {
    m_global_MetricClass_filter.add(filter);
}

void TraceWriter::registerMetricInstanceGlobalFilter(
    GlobalMetricInstanceFilter filter) {
    m_global_MetricInstance_filter.add(filter);
}

void TraceWriter::registerCommGlobalFilter(GlobalCommFilter filter) {
    m_global_Comm_filter.add(filter);
}

void TraceWriter::registerParameterGlobalFilter(GlobalParameterFilter filter) {
    m_global_Parameter_filter.add(filter);
}

void TraceWriter::registerRmaWinGlobalFilter(GlobalRmaWinFilter filter) {
    m_global_RmaWin_filter.add(filter);
}

void TraceWriter::registerMetricClassRecorderGlobalFilter(
    GlobalMetricClassRecorderFilter filter) {
    m_global_MetricClassRecorder_filter.add(filter);
}

void TraceWriter::registerSystemTreeNodePropertyGlobalFilter(
    GlobalSystemTreeNodePropertyFilter filter) {
    m_global_SystemTreeNodeProperty_filter.add(filter);
}

void TraceWriter::registerSystemTreeNodeDomainGlobalFilter(
    GlobalSystemTreeNodeDomainFilter filter) {
    m_global_SystemTreeNodeDomain_filter.add(filter);
}

void TraceWriter::registerLocationGroupPropertyGlobalFilter(
    GlobalLocationGroupPropertyFilter filter) {
    m_global_LocationGroupProperty_filter.add(filter);
}

void TraceWriter::registerLocationPropertyGlobalFilter(
    GlobalLocationPropertyFilter filter) {
    m_global_LocationProperty_filter.add(filter);
}

void TraceWriter::registerCartDimensionGlobalFilter(
    GlobalCartDimensionFilter filter) {
    m_global_CartDimension_filter.add(filter);
}

void TraceWriter::registerCartTopologyGlobalFilter(
    GlobalCartTopologyFilter filter) {
    m_global_CartTopology_filter.add(filter);
}

void TraceWriter::registerCartCoordinateGlobalFilter(
    GlobalCartCoordinateFilter filter) {
    m_global_CartCoordinate_filter.add(filter);
}

void TraceWriter::registerSourceCodeLocationGlobalFilter(
    GlobalSourceCodeLocationFilter filter) {
    m_global_SourceCodeLocation_filter.add(filter);
}

void TraceWriter::registerCallingContextGlobalFilter(
    GlobalCallingContextFilter filter) {
    m_global_CallingContext_filter.add(filter);
}

void TraceWriter::registerCallingContextPropertyGlobalFilter(
    GlobalCallingContextPropertyFilter filter) {
    m_global_CallingContextProperty_filter.add(filter);
}

void TraceWriter::registerInterruptGeneratorGlobalFilter(
    GlobalInterruptGeneratorFilter filter) {
    m_global_InterruptGenerator_filter.add(filter);
}

void TraceWriter::registerIoFilePropertyGlobalFilter(
    GlobalIoFilePropertyFilter filter) {
    m_global_IoFileProperty_filter.add(filter);
}

void TraceWriter::registerIoRegularFileGlobalFilter(
    GlobalIoRegularFileFilter filter) {
    m_global_IoRegularFile_filter.add(filter);
}

void TraceWriter::registerIoDirectoryGlobalFilter(
    GlobalIoDirectoryFilter filter) {
    m_global_IoDirectory_filter.add(filter);
}

void TraceWriter::registerIoHandleGlobalFilter(GlobalIoHandleFilter filter) {
    m_global_IoHandle_filter.add(filter);
}

void TraceWriter::registerIoPreCreatedHandleStateGlobalFilter(
    GlobalIoPreCreatedHandleStateFilter filter) {
    m_global_IoPreCreatedHandleState_filter.add(filter);
}

void TraceWriter::registerCallpathParameterGlobalFilter(
    GlobalCallpathParameterFilter filter) {
    m_global_CallpathParameter_filter.add(filter);
}

void TraceWriter::registerMappingTableLocalFilter(
    LocalMappingTableFilter filter) {
    m_local_MappingTable_filter.add(filter);
}

void TraceWriter::registerClockOffsetLocalFilter(
    LocalClockOffsetFilter filter) {
    m_local_ClockOffset_filter.add(filter);
}

void TraceWriter::registerStringLocalFilter(LocalStringFilter filter) {
    m_local_String_filter.add(filter);
}

void TraceWriter::registerAttributeLocalFilter(LocalAttributeFilter filter) {
    m_local_Attribute_filter.add(filter);
}

void TraceWriter::registerSystemTreeNodeLocalFilter(
    LocalSystemTreeNodeFilter filter) {
    m_local_SystemTreeNode_filter.add(filter);
}

void TraceWriter::registerLocationGroupLocalFilter(
    LocalLocationGroupFilter filter) {
    m_local_LocationGroup_filter.add(filter);
}

void TraceWriter::registerLocationLocalFilter(LocalLocationFilter filter) {
    m_local_Location_filter.add(filter);
}

void TraceWriter::registerRegionLocalFilter(LocalRegionFilter filter) {
    m_local_Region_filter.add(filter);
}

void TraceWriter::registerCallsiteLocalFilter(LocalCallsiteFilter filter) {
    m_local_Callsite_filter.add(filter);
}

void TraceWriter::registerCallpathLocalFilter(LocalCallpathFilter filter) {
    m_local_Callpath_filter.add(filter);
}

void TraceWriter::registerGroupLocalFilter(LocalGroupFilter filter) {
    m_local_Group_filter.add(filter);
}

void TraceWriter::registerMetricMemberLocalFilter(
    LocalMetricMemberFilter filter) {
    m_local_MetricMember_filter.add(filter);
}

void TraceWriter::registerMetricClassLocalFilter(
    LocalMetricClassFilter filter) {
    m_local_MetricClass_filter.add(filter);
}

void TraceWriter::registerMetricInstanceLocalFilter(
    LocalMetricInstanceFilter filter) {
    m_local_MetricInstance_filter.add(filter);
}

void TraceWriter::registerCommLocalFilter(LocalCommFilter filter) {
    m_local_Comm_filter.add(filter);
}

void TraceWriter::registerParameterLocalFilter(LocalParameterFilter filter) {
    m_local_Parameter_filter.add(filter);
}

void TraceWriter::registerRmaWinLocalFilter(LocalRmaWinFilter filter) {
    m_local_RmaWin_filter.add(filter);
}

void TraceWriter::registerMetricClassRecorderLocalFilter(
    LocalMetricClassRecorderFilter filter) {
    m_local_MetricClassRecorder_filter.add(filter);
}

void TraceWriter::registerSystemTreeNodePropertyLocalFilter(
    LocalSystemTreeNodePropertyFilter filter) {
    m_local_SystemTreeNodeProperty_filter.add(filter);
}

void TraceWriter::registerSystemTreeNodeDomainLocalFilter(
    LocalSystemTreeNodeDomainFilter filter) {
    m_local_SystemTreeNodeDomain_filter.add(filter);
}

void TraceWriter::registerLocationGroupPropertyLocalFilter(
    LocalLocationGroupPropertyFilter filter) {
    m_local_LocationGroupProperty_filter.add(filter);
}

void TraceWriter::registerLocationPropertyLocalFilter(
    LocalLocationPropertyFilter filter) {
    m_local_LocationProperty_filter.add(filter);
}

void TraceWriter::registerCartDimensionLocalFilter(
    LocalCartDimensionFilter filter) {
    m_local_CartDimension_filter.add(filter);
}

void TraceWriter::registerCartTopologyLocalFilter(
    LocalCartTopologyFilter filter) {
    m_local_CartTopology_filter.add(filter);
}

void TraceWriter::registerCartCoordinateLocalFilter(
    LocalCartCoordinateFilter filter) {
    m_local_CartCoordinate_filter.add(filter);
}

void TraceWriter::registerSourceCodeLocationLocalFilter(
    LocalSourceCodeLocationFilter filter) {
    m_local_SourceCodeLocation_filter.add(filter);
}

void TraceWriter::registerCallingContextLocalFilter(
    LocalCallingContextFilter filter) {
    m_local_CallingContext_filter.add(filter);
}

void TraceWriter::registerCallingContextPropertyLocalFilter(
    LocalCallingContextPropertyFilter filter) {
    m_local_CallingContextProperty_filter.add(filter);
}

void TraceWriter::registerInterruptGeneratorLocalFilter(
    LocalInterruptGeneratorFilter filter) {
    m_local_InterruptGenerator_filter.add(filter);
}

void TraceWriter::registerIoFilePropertyLocalFilter(
    LocalIoFilePropertyFilter filter) {
    m_local_IoFileProperty_filter.add(filter);
}

void TraceWriter::registerIoRegularFileLocalFilter(
    LocalIoRegularFileFilter filter) {
    m_local_IoRegularFile_filter.add(filter);
}

void TraceWriter::registerIoDirectoryLocalFilter(
    LocalIoDirectoryFilter filter) {
    m_local_IoDirectory_filter.add(filter);
}

void TraceWriter::registerIoHandleLocalFilter(LocalIoHandleFilter filter) {
    m_local_IoHandle_filter.add(filter);
}

void TraceWriter::registerIoPreCreatedHandleStateLocalFilter(
    LocalIoPreCreatedHandleStateFilter filter) {
    m_local_IoPreCreatedHandleState_filter.add(filter);
}

void TraceWriter::registerCallpathParameterLocalFilter(
    LocalCallpathParameterFilter filter) {
    m_local_CallpathParameter_filter.add(filter);
}

void TraceWriter::registerBufferFlushEventFilter(
    EventBufferFlushFilter filter) {
    m_event_BufferFlush_filter.add(filter);
}

void TraceWriter::registerMeasurementOnOffEventFilter(
    EventMeasurementOnOffFilter filter) {
    m_event_MeasurementOnOff_filter.add(filter);
}

void TraceWriter::registerEnterEventFilter(EventEnterFilter filter) {
    m_event_Enter_filter.add(filter);
}

void TraceWriter::registerLeaveEventFilter(EventLeaveFilter filter) {
    m_event_Leave_filter.add(filter);
}

void TraceWriter::registerMpiSendEventFilter(EventMpiSendFilter filter) {
    m_event_MpiSend_filter.add(filter);
}

void TraceWriter::registerMpiIsendEventFilter(EventMpiIsendFilter filter) {
    m_event_MpiIsend_filter.add(filter);
}

void TraceWriter::registerMpiIsendCompleteEventFilter(
    EventMpiIsendCompleteFilter filter) {
    m_event_MpiIsendComplete_filter.add(filter);
}

void TraceWriter::registerMpiIrecvRequestEventFilter(
    EventMpiIrecvRequestFilter filter) {
    m_event_MpiIrecvRequest_filter.add(filter);
}

void TraceWriter::registerMpiRecvEventFilter(EventMpiRecvFilter filter) {
    m_event_MpiRecv_filter.add(filter);
}

void TraceWriter::registerMpiIrecvEventFilter(EventMpiIrecvFilter filter) {
    m_event_MpiIrecv_filter.add(filter);
}

void TraceWriter::registerMpiRequestTestEventFilter(
    EventMpiRequestTestFilter filter) {
    m_event_MpiRequestTest_filter.add(filter);
}

void TraceWriter::registerMpiRequestCancelledEventFilter(
    EventMpiRequestCancelledFilter filter) {
    m_event_MpiRequestCancelled_filter.add(filter);
}

void TraceWriter::registerMpiCollectiveBeginEventFilter(
    EventMpiCollectiveBeginFilter filter) {
    m_event_MpiCollectiveBegin_filter.add(filter);
}

void TraceWriter::registerMpiCollectiveEndEventFilter(
    EventMpiCollectiveEndFilter filter) {
    m_event_MpiCollectiveEnd_filter.add(filter);
}

void TraceWriter::registerOmpForkEventFilter(EventOmpForkFilter filter) {
    m_event_OmpFork_filter.add(filter);
}

void TraceWriter::registerOmpJoinEventFilter(EventOmpJoinFilter filter) {
    m_event_OmpJoin_filter.add(filter);
}

void TraceWriter::registerOmpAcquireLockEventFilter(
    EventOmpAcquireLockFilter filter) {
    m_event_OmpAcquireLock_filter.add(filter);
}

void TraceWriter::registerOmpReleaseLockEventFilter(
    EventOmpReleaseLockFilter filter) {
    m_event_OmpReleaseLock_filter.add(filter);
}

void TraceWriter::registerOmpTaskCreateEventFilter(
    EventOmpTaskCreateFilter filter) {
    m_event_OmpTaskCreate_filter.add(filter);
}

void TraceWriter::registerOmpTaskSwitchEventFilter(
    EventOmpTaskSwitchFilter filter) {
    m_event_OmpTaskSwitch_filter.add(filter);
}

void TraceWriter::registerOmpTaskCompleteEventFilter(
    EventOmpTaskCompleteFilter filter) {
    m_event_OmpTaskComplete_filter.add(filter);
}

void TraceWriter::registerMetricEventFilter(EventMetricFilter filter) {
    m_event_Metric_filter.add(filter);
}

void TraceWriter::registerParameterStringEventFilter(
    EventParameterStringFilter filter) {
    m_event_ParameterString_filter.add(filter);
}

void TraceWriter::registerParameterIntEventFilter(
    EventParameterIntFilter filter) {
    m_event_ParameterInt_filter.add(filter);
}

void TraceWriter::registerParameterUnsignedIntEventFilter(
    EventParameterUnsignedIntFilter filter) {
    m_event_ParameterUnsignedInt_filter.add(filter);
}

void TraceWriter::registerRmaWinCreateEventFilter(
    EventRmaWinCreateFilter filter) {
    m_event_RmaWinCreate_filter.add(filter);
}

void TraceWriter::registerRmaWinDestroyEventFilter(
    EventRmaWinDestroyFilter filter) {
    m_event_RmaWinDestroy_filter.add(filter);
}

void TraceWriter::registerRmaCollectiveBeginEventFilter(
    EventRmaCollectiveBeginFilter filter) {
    m_event_RmaCollectiveBegin_filter.add(filter);
}

void TraceWriter::registerRmaCollectiveEndEventFilter(
    EventRmaCollectiveEndFilter filter) {
    m_event_RmaCollectiveEnd_filter.add(filter);
}

void TraceWriter::registerRmaGroupSyncEventFilter(
    EventRmaGroupSyncFilter filter) {
    m_event_RmaGroupSync_filter.add(filter);
}

void TraceWriter::registerRmaRequestLockEventFilter(
    EventRmaRequestLockFilter filter) {
    m_event_RmaRequestLock_filter.add(filter);
}

void TraceWriter::registerRmaAcquireLockEventFilter(
    EventRmaAcquireLockFilter filter) {
    m_event_RmaAcquireLock_filter.add(filter);
}

void TraceWriter::registerRmaTryLockEventFilter(EventRmaTryLockFilter filter) {
    m_event_RmaTryLock_filter.add(filter);
}

void TraceWriter::registerRmaReleaseLockEventFilter(
    EventRmaReleaseLockFilter filter) {
    m_event_RmaReleaseLock_filter.add(filter);
}

void TraceWriter::registerRmaSyncEventFilter(EventRmaSyncFilter filter) {
    m_event_RmaSync_filter.add(filter);
}

void TraceWriter::registerRmaWaitChangeEventFilter(
    EventRmaWaitChangeFilter filter) {
    m_event_RmaWaitChange_filter.add(filter);
}

void TraceWriter::registerRmaPutEventFilter(EventRmaPutFilter filter) {
    m_event_RmaPut_filter.add(filter);
}

void TraceWriter::registerRmaGetEventFilter(EventRmaGetFilter filter) {
    m_event_RmaGet_filter.add(filter);
}

void TraceWriter::registerRmaAtomicEventFilter(EventRmaAtomicFilter filter) {
    m_event_RmaAtomic_filter.add(filter);
}

void TraceWriter::registerRmaOpCompleteBlockingEventFilter(
    EventRmaOpCompleteBlockingFilter filter) {
    m_event_RmaOpCompleteBlocking_filter.add(filter);
}

void TraceWriter::registerRmaOpCompleteNonBlockingEventFilter(
    EventRmaOpCompleteNonBlockingFilter filter) {
    m_event_RmaOpCompleteNonBlocking_filter.add(filter);
}

void TraceWriter::registerRmaOpTestEventFilter(EventRmaOpTestFilter filter) {
    m_event_RmaOpTest_filter.add(filter);
}

void TraceWriter::registerRmaOpCompleteRemoteEventFilter(
    EventRmaOpCompleteRemoteFilter filter) {
    m_event_RmaOpCompleteRemote_filter.add(filter);
}

void TraceWriter::registerThreadForkEventFilter(EventThreadForkFilter filter) {
    m_event_ThreadFork_filter.add(filter);
}

void TraceWriter::registerThreadJoinEventFilter(EventThreadJoinFilter filter) {
    m_event_ThreadJoin_filter.add(filter);
}

void TraceWriter::registerThreadTeamBeginEventFilter(
    EventThreadTeamBeginFilter filter) {
    m_event_ThreadTeamBegin_filter.add(filter);
}

void TraceWriter::registerThreadTeamEndEventFilter(
    EventThreadTeamEndFilter filter) {
    m_event_ThreadTeamEnd_filter.add(filter);
}

void TraceWriter::registerThreadAcquireLockEventFilter(
    EventThreadAcquireLockFilter filter) {
    m_event_ThreadAcquireLock_filter.add(filter);
}

void TraceWriter::registerThreadReleaseLockEventFilter(
    EventThreadReleaseLockFilter filter) {
    m_event_ThreadReleaseLock_filter.add(filter);
}

void TraceWriter::registerThreadTaskCreateEventFilter(
    EventThreadTaskCreateFilter filter) {
    m_event_ThreadTaskCreate_filter.add(filter);
}

void TraceWriter::registerThreadTaskSwitchEventFilter(
    EventThreadTaskSwitchFilter filter) {
    m_event_ThreadTaskSwitch_filter.add(filter);
}

void TraceWriter::registerThreadTaskCompleteEventFilter(
    EventThreadTaskCompleteFilter filter) {
    m_event_ThreadTaskComplete_filter.add(filter);
}

void TraceWriter::registerThreadCreateEventFilter(
    EventThreadCreateFilter filter) {
    m_event_ThreadCreate_filter.add(filter);
}

void TraceWriter::registerThreadBeginEventFilter(
    EventThreadBeginFilter filter) {
    m_event_ThreadBegin_filter.add(filter);
}

void TraceWriter::registerThreadWaitEventFilter(EventThreadWaitFilter filter) {
    m_event_ThreadWait_filter.add(filter);
}

void TraceWriter::registerThreadEndEventFilter(EventThreadEndFilter filter) {
    m_event_ThreadEnd_filter.add(filter);
}

void TraceWriter::registerCallingContextEnterEventFilter(
    EventCallingContextEnterFilter filter) {
    m_event_CallingContextEnter_filter.add(filter);
}

void TraceWriter::registerCallingContextLeaveEventFilter(
    EventCallingContextLeaveFilter filter) {
    m_event_CallingContextLeave_filter.add(filter);
}

void TraceWriter::registerCallingContextSampleEventFilter(
    EventCallingContextSampleFilter filter) {
    m_event_CallingContextSample_filter.add(filter);
}

void TraceWriter::registerIoCreateHandleEventFilter(
    EventIoCreateHandleFilter filter) {
    m_event_IoCreateHandle_filter.add(filter);
}

void TraceWriter::registerIoDestroyHandleEventFilter(
    EventIoDestroyHandleFilter filter) {
    m_event_IoDestroyHandle_filter.add(filter);
}

void TraceWriter::registerIoDuplicateHandleEventFilter(
    EventIoDuplicateHandleFilter filter) {
    m_event_IoDuplicateHandle_filter.add(filter);
}

void TraceWriter::registerIoSeekEventFilter(EventIoSeekFilter filter) {
    m_event_IoSeek_filter.add(filter);
}

void TraceWriter::registerIoChangeStatusFlagsEventFilter(
    EventIoChangeStatusFlagsFilter filter) {
    m_event_IoChangeStatusFlags_filter.add(filter);
}

void TraceWriter::registerIoDeleteFileEventFilter(
    EventIoDeleteFileFilter filter) {
    m_event_IoDeleteFile_filter.add(filter);
}

void TraceWriter::registerIoOperationBeginEventFilter(
    EventIoOperationBeginFilter filter) {
    m_event_IoOperationBegin_filter.add(filter);
}

void TraceWriter::registerIoOperationTestEventFilter(
    EventIoOperationTestFilter filter) {
    m_event_IoOperationTest_filter.add(filter);
}

void TraceWriter::registerIoOperationIssuedEventFilter(
    EventIoOperationIssuedFilter filter) {
    m_event_IoOperationIssued_filter.add(filter);
}

void TraceWriter::registerIoOperationCompleteEventFilter(
    EventIoOperationCompleteFilter filter) {
    m_event_IoOperationComplete_filter.add(filter);
}

void TraceWriter::registerIoOperationCancelledEventFilter(
    EventIoOperationCancelledFilter filter) {
    m_event_IoOperationCancelled_filter.add(filter);
}

void TraceWriter::registerIoAcquireLockEventFilter(
    EventIoAcquireLockFilter filter) {
    m_event_IoAcquireLock_filter.add(filter);
}

void TraceWriter::registerIoReleaseLockEventFilter(
    EventIoReleaseLockFilter filter) {
    m_event_IoReleaseLock_filter.add(filter);
}

void TraceWriter::registerIoTryLockEventFilter(EventIoTryLockFilter filter) {
    m_event_IoTryLock_filter.add(filter);
}

void TraceWriter::registerProgramBeginEventFilter(
    EventProgramBeginFilter filter) {
    m_event_ProgramBegin_filter.add(filter);
}

void TraceWriter::registerProgramEndEventFilter(EventProgramEndFilter filter) {
    m_event_ProgramEnd_filter.add(filter);
}
