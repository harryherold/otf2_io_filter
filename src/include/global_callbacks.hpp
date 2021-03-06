#ifndef GLOBAL_CALLBACKS_HPP
#define GLOBAL_CALLBACKS_HPP

extern "C"
{
#include <otf2/otf2.h>
}

namespace definition
{

OTF2_CallbackCode
GlobalClockPropertiesCb(void *userData, uint64_t timerResolution, uint64_t globalOffset, uint64_t traceLength);

OTF2_CallbackCode
GlobalParadigmCb(void *userData, OTF2_Paradigm paradigm, OTF2_StringRef name, OTF2_ParadigmClass paradigmClass);

OTF2_CallbackCode
GlobalParadigmPropertyCb(
    void *userData, OTF2_Paradigm paradigm, OTF2_ParadigmProperty property, OTF2_Type type, OTF2_AttributeValue value);

OTF2_CallbackCode
GlobalIoParadigmCb(void *                         userData,
                   OTF2_IoParadigmRef             self,
                   OTF2_StringRef                 identification,
                   OTF2_StringRef                 name,
                   OTF2_IoParadigmClass           ioParadigmClass,
                   OTF2_IoParadigmFlag            ioParadigmFlags,
                   uint8_t                        numberOfProperties,
                   const OTF2_IoParadigmProperty *properties,
                   const OTF2_Type *              types,
                   const OTF2_AttributeValue *    values);

OTF2_CallbackCode
GlobalStringCb(void *userData, OTF2_StringRef self, const char *string);

OTF2_CallbackCode
GlobalAttributeCb(
    void *userData, OTF2_AttributeRef self, OTF2_StringRef name, OTF2_StringRef description, OTF2_Type type);

OTF2_CallbackCode
GlobalSystemTreeNodeCb(void *                 userData,
                       OTF2_SystemTreeNodeRef self,
                       OTF2_StringRef         name,
                       OTF2_StringRef         className,
                       OTF2_SystemTreeNodeRef parent);

OTF2_CallbackCode
GlobalLocationGroupCb(void *                 userData,
                      OTF2_LocationGroupRef  self,
                      OTF2_StringRef         name,
                      OTF2_LocationGroupType locationGroupType,
                      OTF2_SystemTreeNodeRef systemTreeParent);

OTF2_CallbackCode
GlobalLocationCb(void *                userData,
                 OTF2_LocationRef      self,
                 OTF2_StringRef        name,
                 OTF2_LocationType     locationType,
                 uint64_t              numberOfEvents,
                 OTF2_LocationGroupRef locationGroup);

OTF2_CallbackCode
GlobalRegionCb(void *          userData,
               OTF2_RegionRef  self,
               OTF2_StringRef  name,
               OTF2_StringRef  canonicalName,
               OTF2_StringRef  description,
               OTF2_RegionRole regionRole,
               OTF2_Paradigm   paradigm,
               OTF2_RegionFlag regionFlags,
               OTF2_StringRef  sourceFile,
               uint32_t        beginLineNumber,
               uint32_t        endLineNumber);

OTF2_CallbackCode
GlobalCallsiteCb(void *           userData,
                 OTF2_CallsiteRef self,
                 OTF2_StringRef   sourceFile,
                 uint32_t         lineNumber,
                 OTF2_RegionRef   enteredRegion,
                 OTF2_RegionRef   leftRegion);

OTF2_CallbackCode
GlobalCallpathCb(void *userData, OTF2_CallpathRef self, OTF2_CallpathRef parent, OTF2_RegionRef region);

OTF2_CallbackCode
GlobalGroupCb(void *          userData,
              OTF2_GroupRef   self,
              OTF2_StringRef  name,
              OTF2_GroupType  groupType,
              OTF2_Paradigm   paradigm,
              OTF2_GroupFlag  groupFlags,
              uint32_t        numberOfMembers,
              const uint64_t *members);

OTF2_CallbackCode
GlobalMetricMemberCb(void *               userData,
                     OTF2_MetricMemberRef self,
                     OTF2_StringRef       name,
                     OTF2_StringRef       description,
                     OTF2_MetricType      metricType,
                     OTF2_MetricMode      metricMode,
                     OTF2_Type            valueType,
                     OTF2_Base            base,
                     int64_t              exponent,
                     OTF2_StringRef       unit);

OTF2_CallbackCode
GlobalMetricClassCb(void *                      userData,
                    OTF2_MetricRef              self,
                    uint8_t                     numberOfMetrics,
                    const OTF2_MetricMemberRef *metricMembers,
                    OTF2_MetricOccurrence       metricOccurrence,
                    OTF2_RecorderKind           recorderKind);

OTF2_CallbackCode
GlobalMetricInstanceCb(void *           userData,
                       OTF2_MetricRef   self,
                       OTF2_MetricRef   metricClass,
                       OTF2_LocationRef recorder,
                       OTF2_MetricScope metricScope,
                       uint64_t         scope);

OTF2_CallbackCode
GlobalCommCb(void *userData, OTF2_CommRef self, OTF2_StringRef name, OTF2_GroupRef group, OTF2_CommRef parent);

OTF2_CallbackCode
GlobalParameterCb(void *userData, OTF2_ParameterRef self, OTF2_StringRef name, OTF2_ParameterType parameterType);

OTF2_CallbackCode
GlobalRmaWinCb(void *userData, OTF2_RmaWinRef self, OTF2_StringRef name, OTF2_CommRef comm);

OTF2_CallbackCode
GlobalMetricClassRecorderCb(void *userData, OTF2_MetricRef metric, OTF2_LocationRef recorder);

OTF2_CallbackCode
GlobalSystemTreeNodePropertyCb(void *                 userData,
                               OTF2_SystemTreeNodeRef systemTreeNode,
                               OTF2_StringRef         name,
                               OTF2_Type              type,
                               OTF2_AttributeValue    value);

OTF2_CallbackCode
GlobalSystemTreeNodeDomainCb(void *                 userData,
                             OTF2_SystemTreeNodeRef systemTreeNode,
                             OTF2_SystemTreeDomain  systemTreeDomain);

OTF2_CallbackCode
GlobalLocationGroupPropertyCb(void *                userData,
                              OTF2_LocationGroupRef locationGroup,
                              OTF2_StringRef        name,
                              OTF2_Type             type,
                              OTF2_AttributeValue   value);

OTF2_CallbackCode
GlobalLocationPropertyCb(
    void *userData, OTF2_LocationRef location, OTF2_StringRef name, OTF2_Type type, OTF2_AttributeValue value);

OTF2_CallbackCode
GlobalCartDimensionCb(void *                userData,
                      OTF2_CartDimensionRef self,
                      OTF2_StringRef        name,
                      uint32_t              size,
                      OTF2_CartPeriodicity  cartPeriodicity);

OTF2_CallbackCode
GlobalCartTopologyCb(void *                       userData,
                     OTF2_CartTopologyRef         self,
                     OTF2_StringRef               name,
                     OTF2_CommRef                 communicator,
                     uint8_t                      numberOfDimensions,
                     const OTF2_CartDimensionRef *cartDimensions);

OTF2_CallbackCode
GlobalCartCoordinateCb(void *               userData,
                       OTF2_CartTopologyRef cartTopology,
                       uint32_t             rank,
                       uint8_t              numberOfDimensions,
                       const uint32_t *     coordinates);

OTF2_CallbackCode
GlobalSourceCodeLocationCb(void *userData, OTF2_SourceCodeLocationRef self, OTF2_StringRef file, uint32_t lineNumber);

OTF2_CallbackCode
GlobalCallingContextCb(void *                     userData,
                       OTF2_CallingContextRef     self,
                       OTF2_RegionRef             region,
                       OTF2_SourceCodeLocationRef sourceCodeLocation,
                       OTF2_CallingContextRef     parent);

OTF2_CallbackCode
GlobalCallingContextPropertyCb(void *                 userData,
                               OTF2_CallingContextRef callingContext,
                               OTF2_StringRef         name,
                               OTF2_Type              type,
                               OTF2_AttributeValue    value);

OTF2_CallbackCode
GlobalInterruptGeneratorCb(void *                      userData,
                           OTF2_InterruptGeneratorRef  self,
                           OTF2_StringRef              name,
                           OTF2_InterruptGeneratorMode interruptGeneratorMode,
                           OTF2_Base                   base,
                           int64_t                     exponent,
                           uint64_t                    period);

OTF2_CallbackCode
GlobalIoFilePropertyCb(
    void *userData, OTF2_IoFileRef ioFile, OTF2_StringRef name, OTF2_Type type, OTF2_AttributeValue value);

OTF2_CallbackCode
GlobalIoRegularFileCb(void *userData, OTF2_IoFileRef self, OTF2_StringRef name, OTF2_SystemTreeNodeRef scope);

OTF2_CallbackCode
GlobalIoDirectoryCb(void *userData, OTF2_IoFileRef self, OTF2_StringRef name, OTF2_SystemTreeNodeRef scope);

OTF2_CallbackCode
GlobalIoHandleCb(void *             userData,
                 OTF2_IoHandleRef   self,
                 OTF2_StringRef     name,
                 OTF2_IoFileRef     file,
                 OTF2_IoParadigmRef ioParadigm,
                 OTF2_IoHandleFlag  ioHandleFlags,
                 OTF2_CommRef       comm,
                 OTF2_IoHandleRef   parent);

OTF2_CallbackCode
GlobalIoPreCreatedHandleStateCb(void *            userData,
                                OTF2_IoHandleRef  ioHandle,
                                OTF2_IoAccessMode mode,
                                OTF2_IoStatusFlag statusFlags);

OTF2_CallbackCode
GlobalCallpathParameterCb(
    void *userData, OTF2_CallpathRef callpath, OTF2_ParameterRef parameter, OTF2_Type type, OTF2_AttributeValue value);

} // namespace definition

#endif /* GLOBAL_CALLBACKS_HPP */