#include "resolverTokens.h"

#include "pxr/pxr.h"
#include <pxr/base/tf/token.h>
#include <pxr/external/boost/python/class.hpp>
#include <pxr/external/boost/python/return_value_policy.hpp>
#include <pxr/external/boost/python/make_function.hpp>
#include <pxr/external/boost/python/return_by_value.hpp>
#include <pxr/external/boost/python/noncopyable.hpp>
#include <pxr/external/boost/python/init.hpp>
#include <pxr/external/boost/python/type_list.hpp>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


namespace {

class _WrapStaticToken {
public:
    explicit _WrapStaticToken(const TfToken* token)
        : _token(token) {}

    std::string operator()() const { return _token->GetString(); }

private:
    const TfToken* _token;
};

template<typename T>
void _AddToken(T &cls, const char* name, const TfToken &token) {
    cls.add_static_property(name, make_function(
        _WrapStaticToken(&token),
        return_value_policy<return_by_value>(),
        type_list<std::string>()
    ));
}

} // namespace

void wrapResolverTokens() {
    class_<AyonUsdResolverTokensType, noncopyable>("Tokens", no_init);
}
