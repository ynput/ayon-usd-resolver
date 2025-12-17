// #include "../resolver.h"

// #include <pxr/pxr.h>
// #include <pxr/usd/ar/resolver.h>

// #include "boost_include_wrapper.h"
// // clang-format off
// #include BOOST_INCLUDE(python/class.hpp)
// #include BOOST_INCLUDE(python/operators.hpp)
// #include BOOST_INCLUDE(python/return_value_policy.hpp)
// #include BOOST_INCLUDE(python/copy_const_reference.hpp)
// // clang-format on
// using namespace AR_BOOST_NAMESPACE::python;

#include "../resolver.h"

#include <pxr/pxr.h>
#include <pxr/usd/ar/resolver.h>

// FIXED: Necessary headers for internal Boost/Python
// #include <pxr/base/tf/pyModule.h>
#include <pxr/external/boost/python/class.hpp>
#include <pxr/external/boost/python/operators.hpp>
#include <pxr/external/boost/python/return_value_policy.hpp>
#include <pxr/external/boost/python/copy_const_reference.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python; 

void
wrapResolver() {
    using This = AyonUsdResolver;

    // Revert to using the simplified types because of the 'using namespace' statement above.
    class_<This, bases<ArResolver>, noncopyable>("Resolver")
        .def("GetConnectedContext", &This::GetConnectedContext, return_value_policy<reference_existing_object>(), "");
}
