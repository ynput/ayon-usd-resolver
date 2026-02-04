#include "../resolverContext.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/pyUtils.h"
#include <pxr/external/boost/python/class.hpp>
#include <pxr/external/boost/python/operators.hpp>
#include <pxr/external/boost/python/return_value_policy.hpp>
#include "pxr/usd/ar/pyResolverContext.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static size_t
_Hash(const AyonUsdResolverContext &ctx) {
    return hash_value(ctx);
}

static std::string
_Repr(const AyonUsdResolverContext &ctx) {
    return TF_PY_REPR_PREFIX + "ResolverContext"; 
}

void
wrapResolverContext() {
    using This = AyonUsdResolverContext;

    class_<This>("ResolverContext")
        // Constructors
        .def(init<>())
        .def(init<const std::string&>(args("mappingFile")))
        
        // Comparison operators
        .def(self == self)
        .def(self != self)
        .def("__hash__", _Hash)
        .def("__repr__", _Repr)
        
        // Existing cache methods
        .def("ClearAndReinitialize", &This::ClearAndReinitialize, 
             "Currently a no op")
        .def("DropCache", &This::DropCache,
             "Disconnects the currently used std::shared_ptr from the context class and generates a new Cache Class instance")
        .def("DeleteFromCache", &This::DeleteFromCache,
             "Deletes cached entity from the Connected Cache Class instance.")
        .def("ClearCache", &This::ClearCache, 
             "Deletes all cached entities from the Connected Cache Class instance.")
        
        // Mapping file path methods
        .def("GetMappingFilePath", &This::GetMappingFilePath, 
             return_value_policy<return_by_value>(), 
             "Get the mapping file path")
        .def("SetMappingFilePath", &This::SetMappingFilePath, 
             "Set the mapping file path")
        .def("RefreshFromMappingFilePath", &This::RefreshFromMappingFilePath, 
             "Reload mapping pairs from the mapping file path")
        
        // Mapping pair methods
        .def("GetMappingPairs", &This::GetMappingPairs, 
             return_value_policy<return_by_value>(), 
             "Returns all mapping pairs as a dict")
        .def("AddMappingPair", &This::AddMappingPair, 
             "Add a mapping pair (source, target)")
        .def("RemoveMappingByKey", &This::RemoveMappingByKey, 
             "Remove a mapping pair by source key")
        .def("RemoveMappingByValue", &This::RemoveMappingByValue, 
             "Remove a mapping pair by target value")
        .def("ClearMappingPairs", &This::ClearMappingPairs, 
             "Clear all mapping pairs");

    ArWrapResolverContextForPython<This>();
}
