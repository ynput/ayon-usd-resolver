#include "../ayonFileHandler/ayonFileHandler.h"

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
wrapAyonFileHanlder() {
    using This = ayonDataFileHandler;

    class_<This>("AyonDataFileHandler", no_init);
}
