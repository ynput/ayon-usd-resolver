#include "assetIdentifierDef.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

namespace {

/**
 * @brief Expands environment variables and ~ in a file-system path.
 *
 * Supported formats:
 *   ~/path          — replaced with HOME (or USERPROFILE on Windows)
 *   $VAR/path       — bare dollar-sign variable
 *   ${VAR}/path     — brace-delimited variable
 */
static std::string
expandPathVars(const std::string &path) {
    if (path.empty()) {
        return path;
    }

    std::string result;
    result.reserve(path.size());

    size_t i = 0;

    // Expand leading ~
    if (path[0] == '~' && (path.size() == 1 || path[1] == '/' || path[1] == '\\')) {
        const char *home = std::getenv("HOME");
#ifdef _WIN32
        if (!home) {
            home = std::getenv("USERPROFILE");
        }
#endif
        if (home) {
            result += home;
        }
        else {
            result += '~';
        }
        i = 1;
    }

    while (i < path.size()) {
        if (path[i] == '$') {
            size_t j = i + 1;
            if (j < path.size() && path[j] == '{') {
                // ${VAR} form
                ++j;
                size_t end = path.find('}', j);
                if (end != std::string::npos) {
                    std::string varName = path.substr(j, end - j);
                    const char *val = std::getenv(varName.c_str());
                    if (val) {
                        result += val;
                    }
                    i = end + 1;
                    continue;
                }
            }
            else {
                // $VAR form — collect [A-Za-z0-9_] characters
                size_t end = j;
                while (end < path.size()
                       && (std::isalnum(static_cast<unsigned char>(path[end])) || path[end] == '_')) {
                    ++end;
                }
                if (end > j) {
                    std::string varName = path.substr(j, end - j);
                    const char *val = std::getenv(varName.c_str());
                    if (val) {
                        result += val;
                    }
                    i = end;
                    continue;
                }
            }
        }
        result += path[i++];
    }

    return result;
}

} // namespace

PXR_NAMESPACE_USING_DIRECTIVE

ArResolvedPath
AssetIdentifier::getResolvedAssetPath() const {
    return m_resolvedAssetPath;
};

bool
AssetIdentifier::setResolvedAssetPath(const ArResolvedPath &inResolvedAssetPath) {
    if (!isModifiable()) {
        return false;
    }
    m_resolvedAssetPath = inResolvedAssetPath;
    return true;
};
bool
AssetIdentifier::setResolvedAssetPath(const std::string &inResolvedAssetPath) {
    if (!isModifiable()) {
        return false;
    }
    m_resolvedAssetPath = ArResolvedPath(expandPathVars(inResolvedAssetPath));
    return true;
};

std::string
AssetIdentifier::getAssetIdentifier() const {
    return m_assetIdentifier;
};

bool
AssetIdentifier::setAssetIdentifier(const std::string &inAssetIdentifier) {
    if (!isModifiable()) {
        return false;
    }
    m_assetIdentifier = inAssetIdentifier;
    return true;
};

bool
AssetIdentifier::isEmpty() const {
    if (m_assetIdentifier.empty() && m_resolvedAssetPath.empty()) {
        return true;
    }

    return false;
};

bool
AssetIdentifier::isValid() const {
    return !m_invalidated;
};

void
AssetIdentifier::invalidate() {
    if (!isModifiable()) {
        return;
    };

    m_invalidated = true;
};

void
AssetIdentifier::validate() {
    if (!isModifiable()) {
        return;
    };

    m_invalidated = false;
};

bool
AssetIdentifier::isModifiable() const {
    return !m_static;
};

void
AssetIdentifier::printInfo() const {
    std::ostringstream oss;
    oss << static_cast<const void*>(this);
    std::cout << "Static; " << m_static << " invalidated; " << m_invalidated << " AssetIdentifier; "
              << m_assetIdentifier << " ResolvedPath; " << m_resolvedAssetPath.GetPathString().c_str()
              << " Instance_m_Pose; " << oss.str().c_str() << " Instance_m_Size; "
              << std::to_string(sizeof(*this)).c_str() << std::endl;
};

bool
AssetIdentifier::operator==(const AssetIdentifier &other) const {
    return m_assetIdentifier == other.m_assetIdentifier;
};

AssetIdentifier &
AssetIdentifier::operator=(const AssetIdentifier &other) {
    if (this != &other) {
        m_resolvedAssetPath = other.m_resolvedAssetPath;
        m_assetIdentifier = other.m_assetIdentifier;
        m_invalidated = other.m_invalidated;
    }
    return *this;
}
