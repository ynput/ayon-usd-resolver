#include "../resolver.h"

#include <pxr/pxr.h>
#include <pxr/usd/ar/resolver.h>

#include "boost_include_wrapper.h"
// clang-format off
#include BOOST_INCLUDE(python/class.hpp)
#include BOOST_INCLUDE(python/operators.hpp)
#include BOOST_INCLUDE(python/return_value_policy.hpp)
#include BOOST_INCLUDE(python/copy_const_reference.hpp)
// clang-format on
using namespace AR_BOOST_NAMESPACE::python;

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapResolver() {
    using This = AyonUsdResolver;

    class_<This, bases<ArResolver>, AR_BOOST_NAMESPACE::noncopyable>("Resolver", no_init)
        .def("GetConnectedContext", &This::GetConnectedContext, return_value_policy<reference_existing_object>(), "");
}
