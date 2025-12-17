#include "../resolver.h"

#include <pxr/pxr.h>
#include <pxr/external/boost/python/class.hpp>
#include <pxr/external/boost/python/copy_const_reference.hpp>
#include <pxr/external/boost/python/operators.hpp>
#include <pxr/external/boost/python/return_value_policy.hpp>
#include <pxr/usd/ar/resolver.h>

PXR_NAMESPACE_USING_DIRECTIVE

// Use the Pixar-vendored Boost Python namespace
using namespace pxr_boost::python;

void wrapResolver() {
    using This = AyonUsdResolver;

    class_<This, bases<ArResolver>, noncopyable>("Resolver", no_init)
        .def("GetConnectedContext", 
             &This::GetConnectedContext, 
             return_value_policy<reference_existing_object>(), 
             "");
}
