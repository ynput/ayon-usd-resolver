#include "assetIdentifierDef.h"

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

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
    m_resolvedAssetPath = ArResolvedPath(inResolvedAssetPath);
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
