#include "resolverTokens.h"

#include "pxr/pxr.h"
#include <pxr/base/tf/token.h>

#ifdef AYON_USE_HBOOST
    #include <hboost/python/class.hpp>
    #include <hboost/python/return_value_policy.hpp>
    #include <hboost/python/make_function.hpp>
    #include <hboost/python/return_by_value.hpp>
    #include <hboost/python/init.hpp>
    #include <hboost/noncopyable.hpp>
    #include <hboost/mpl/vector.hpp>
    namespace boost_python_ns = hboost::python;
    using boost_noncopyable = hboost::noncopyable;
    template<typename... Args>
    using boost_type_list = hboost::mpl::vector<Args...>;
#elif defined(AYON_USE_BOOST)
    #include <boost/python/class.hpp>
    #include <boost/python/return_value_policy.hpp>
    #include <boost/python/make_function.hpp>
    #include <boost/python/return_by_value.hpp>
    #include <boost/python/init.hpp>
    #include <boost/noncopyable.hpp>
    #include <boost/mpl/vector.hpp>
    namespace boost_python_ns = boost::python;
    using boost_noncopyable = boost::noncopyable;
    template<typename... Args>
    using boost_type_list = boost::mpl::vector<Args...>;
#else
    #include <pxr/external/boost/python/class.hpp>
    #include <pxr/external/boost/python/return_value_policy.hpp>
    #include <pxr/external/boost/python/make_function.hpp>
    #include <pxr/external/boost/python/return_by_value.hpp>
    #include <pxr/external/boost/python/noncopyable.hpp>
    #include <pxr/external/boost/python/init.hpp>
    #include <pxr/external/boost/python/type_list.hpp>
    namespace boost_python_ns = PXR_NS::pxr_boost::python;
    using boost_noncopyable = PXR_NS::pxr_boost::python::noncopyable;
    template<typename... Args>
    using boost_type_list = PXR_NS::pxr_boost::python::type_list<Args...>;
#endif

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost_python_ns;


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
        boost_type_list<std::string>()
    ));
}

} // namespace

void wrapResolverTokens() {
    class_<AyonUsdResolverTokensType, boost_noncopyable>("Tokens", no_init);
}
