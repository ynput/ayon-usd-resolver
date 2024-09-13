#include "../resolverContext.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/pxr.h"
#include "pxr/usd/ar/pyResolverContext.h"

#include "boost_include_wrapper.h"
// clang-format off
#include BOOST_INCLUDE(python/class.hpp)
#include BOOST_INCLUDE(python/operators.hpp)
#include BOOST_INCLUDE(python/return_value_policy.hpp)
// clang-format on
#include <string>

using namespace AR_BOOST_NAMESPACE::python;

PXR_NAMESPACE_USING_DIRECTIVE

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

    ArWrapResolverContextForPython<This>();
}
