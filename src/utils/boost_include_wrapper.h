#ifndef BOOST_INCLUDE_WRAPPER_H
#define BOOST_INCLUDE_WRAPPER_H
#ifndef AR_BOOST_NAMESPACE
    #define AR_BOOST_NAMESPACE boost
#endif

#define BOOST_INCLUDE(path) <AR_BOOST_NAMESPACE/path>

// #define BOOST_INCLUDE(path) <CONCATINATE(AR_BOOST_NAMESPACE, path)>
#endif   // BOOST_INCLUDE_WRAPPER_H
