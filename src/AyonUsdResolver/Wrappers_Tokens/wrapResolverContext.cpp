#include "../resolverContext.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/pyUtils.h"

#ifdef AYON_USE_HBOOST
    #include <hboost/python/class.hpp>
    #include <hboost/python/operators.hpp>
    #include <hboost/python/return_value_policy.hpp>
    #include <hboost/python/self.hpp>
    namespace boost_python_ns = hboost::python;
#else
    #include <pxr/external/boost/python/class.hpp>
    #include <pxr/external/boost/python/operators.hpp>
    #include <pxr/external/boost/python/return_value_policy.hpp>
    namespace boost_python_ns = pxr_boost::python;
#endif

#include "pxr/usd/ar/pyResolverContext.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost_python_ns;

static size_t
_Hash(const AyonUsdResolverContext &ctx) {
    // This function requires pxr/base/tf/hash.h, which is likely included via a pxr header.
    return hash_value(ctx);
}

static std::string
_Repr(const AyonUsdResolverContext &ctx) {
    // Use the simpler representation helper, assuming Tf/pyUtils.h defines TF_PY_REPR_PREFIX.
    return TF_PY_REPR_PREFIX + "ResolverContext"; 
}

void
wrapResolverContext() {
    using This = AyonUsdResolverContext;

    // FIX: Revert to simplified names, relying on 'using namespace pxr_boost::python;'
    class_<This>("ResolverContext")
        .def(self == self)
        .def(self != self)
        .def("__hash__", _Hash)
        .def("__repr__", _Repr)
        .def("ClearAndReinitialize", &This::ClearAndReinitialize, "Currently a no op")

        .def("dropCache", &This::dropCache,
             "disconnects the currently used std::shared_ptr from the context class and generates a new Cache Class "
             "instance")
        .def("deleteFromCache", &This::deleteFromCache,
             "Deletes cached entity from the Connected Cache Class instance.")
        .def("clearCache", &This::clearCache, "Deletes all cached entities from the Connected Cache Class instance.");

    // This should resolve correctly if the PXR_NAMESPACE_USING_DIRECTIVE is used.
    // PXR_NS::ArWrapResolverContextForPython<This>();
    ArWrapResolverContextForPython<This>();
}
