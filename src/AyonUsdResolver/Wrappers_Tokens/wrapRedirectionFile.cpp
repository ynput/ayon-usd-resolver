#include <netinet/tcp.h>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>

#include "../redirectionFileHanlder/redirectionHanlder.h"

#include "pxr/base/tf/pyUtils.h"

#include "boost_include_wrapper.h"
// clang-format off
#include BOOST_INCLUDE(python/def.hpp)
#include BOOST_INCLUDE(python/class.hpp)
#include BOOST_INCLUDE(python/operators.hpp)
#include BOOST_INCLUDE(python/return_value_policy.hpp)
#include BOOST_INCLUDE(python/copy_const_reference.hpp)
#include BOOST_INCLUDE(python/dict.hpp)
// clang-format on

using namespace AR_BOOST_NAMESPACE::python;

// PXR_NAMESPACE_USING_DIRECTIVE

static size_t
_Hash(const redirectionFile &rfl) {
    return std::hash<std::string>{}(rfl.getLayers().at(0).string());
}

static std::string
_Repr(const redirectionFile &rfl) {
    return TF_PY_REPR_PREFIX + "redirectionFile";
}

AR_BOOST_NAMESPACE::python::list
_getLayers(const redirectionFile &rfl) {
    AR_BOOST_NAMESPACE::python::list pyList;
    for (const std::filesystem::path &p: rfl.getLayers()) {
        pyList.append(p.string());
    }
    return pyList;
}

AR_BOOST_NAMESPACE::python::dict
_getRedirections(const redirectionFile &rfl) {
    AR_BOOST_NAMESPACE::python::dict pyDict;
    for (const std::pair<std::string, std::string> &p: rfl.getRedirections()) {
        pyDict[p.first] = p.second;
    }
    return pyDict;
}

void
wrapredirectionFile() {
    using This = redirectionFile;
    class_<This, AR_BOOST_NAMESPACE::noncopyable>("RedirectionFile", no_init)
        .def(init<const std::string &>(args("entryFile")))
        .def(self == self)
        .def(self != self)
        .def("__hash__", _Hash)
        .def("__repr__", _Repr)
        .def("getLayers", _getLayers)
        .def("addLayer", &This::addLayerStr)
        .def("reload", &This::reload, "reloads the Redirection Stack from disk without saving the current state")
        .def("save", &This::save)
        .def("addRedirection", &This::addRedirection)
        .def("saveToFile", &This::saveToFile)
        .def("getRedirections", _getRedirections)
        .def("removeLayer", &This::removeLayer)
        .def("removeRedirection", &This::removeRedirection)
        .def("clearLayers", &This::clearLayers)
        .def("clearRedirections", &This::clearRedirections);
}

redirectionFile*
_GetRdFilePy(const std::string &entryFile) {
    return getRdFile(std::filesystem::path(entryFile).string());
}

void
wrapgetRDF() {
    def("getRdFile", _GetRdFilePy, return_value_policy<reference_existing_object>());
}
