#include "pxr/base/tf/pyModule.h"

TF_WRAP_MODULE {
    TF_WRAP(Resolver);
    TF_WRAP(ResolverContext);
    TF_WRAP(ResolverTokens);

    TF_WRAP(redirectionFile);
    TF_WRAP(getRDF);
}
