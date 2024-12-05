#include <filesystem>
#include <functional>
#include <string>

#include "../redirectionFileHanlder/redirectionHanlder.h"

#include "pxr/base/tf/pyUtils.h"

#include "boost_include_wrapper.h"
// clang-format off
#include BOOST_INCLUDE(python/def.hpp)
#include BOOST_INCLUDE(python/class.hpp)
#include BOOST_INCLUDE(python/operators.hpp)
#include BOOST_INCLUDE(python/return_value_policy.hpp)
#include BOOST_INCLUDE(python/copy_const_reference.hpp)
// clang-format on

using namespace AR_BOOST_NAMESPACE::python;

// PXR_NAMESPACE_USING_DIRECTIVE

static size_t
_Hash(const redirectionFile &rfl) {
    return std::hash<std::string>{}(rfl.getLayers().at(0));
}

static std::string
_Repr(const redirectionFile &rfl) {
    return TF_PY_REPR_PREFIX + "redirectionFile";
}

AR_BOOST_NAMESPACE::python::list
_getLayers(const redirectionFile &rfl) {
    AR_BOOST_NAMESPACE::python::list pyList;
    for (const auto &p: rfl.getLayers()) {
        pyList.append(p.string());
    }
    return pyList;
}

void
wrapredirectionFile() {
    using This = redirectionFile;
    class_<This, AR_BOOST_NAMESPACE::noncopyable>("RedirectionFile", no_init)
        .def(init<const std::filesystem::path &>(args("entryFile")))
        .def(init<const std::string &>(args("entryFile")))
        .def(self == self)
        .def(self != self)
        .def("__hash__", _Hash)
        .def("__repr__", _Repr)
        .def("getLayers", _getLayers)
        .def("addLayer", &This::addLayerStr)
        .def("reload", &This::reload, "reloads the Redirection Stack from disk without saving the current state")
        .def("save", &This::save)
        .def("addRedirection", &This::addRedirection);
}

redirectionFile*
_GetRdFilePy(const std::string &entryFile) {
    return getRdFile(std::filesystem::path(entryFile));
}

void
wrapgetRDF() {
    def("getRdFile", _GetRdFilePy, return_value_policy<reference_existing_object>());
}
