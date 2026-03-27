#include "../resolver.h"

#include <pxr/pxr.h>

#ifdef AYON_USE_HBOOST
    #include <hboost/python/class.hpp>
    #include <hboost/python/copy_const_reference.hpp>
    #include <hboost/python/operators.hpp>
    #include <hboost/python/return_value_policy.hpp>
    #include <hboost/noncopyable.hpp>
    namespace boost_python_ns = hboost::python;
    using boost_noncopyable = hboost::noncopyable;
#elif defined(AYON_USE_BOOST)
    #include <boost/python/class.hpp>
    #include <boost/python/copy_const_reference.hpp>
    #include <boost/python/operators.hpp>
    #include <boost/python/return_value_policy.hpp>
    #include <boost/noncopyable.hpp>
    namespace boost_python_ns = boost::python;
    using boost_noncopyable = boost::noncopyable;
#else
    #include <pxr/external/boost/python/class.hpp>
    #include <pxr/external/boost/python/copy_const_reference.hpp>
    #include <pxr/external/boost/python/operators.hpp>
    #include <pxr/external/boost/python/return_value_policy.hpp>
    namespace boost_python_ns = PXR_NS::pxr_boost::python;
    using boost_noncopyable = PXR_NS::pxr_boost::python::noncopyable;
#endif

#include <pxr/usd/ar/resolver.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost_python_ns;

void wrapResolver() {
    using This = AyonUsdResolver;

    class_<This, bases<ArResolver>, boost_noncopyable>("Resolver", no_init)
        .def("GetConnectedContext",
             &This::GetConnectedContext,
             return_value_policy<reference_existing_object>(),
             "");
}
