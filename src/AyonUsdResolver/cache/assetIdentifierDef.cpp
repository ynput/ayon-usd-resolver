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
    if (!this->is_modifiable()) {
        return false;
    }
    this->m_resolvedAssetPath = inResolvedAssetPath;
    return true;
};
bool
AssetIdentifier::setResolvedAssetPath(const std::string &inResolvedAssetPath) {
    if (!this->is_modifiable()) {
        return false;
    }
    this->m_resolvedAssetPath = ArResolvedPath(inResolvedAssetPath);
    return true;
};

std::string
AssetIdentifier::getAssetIdentifier() const {
    return m_assetIdentifier;
};

bool
AssetIdentifier::setAssetIdentifier(const std::string &inAssetIdentifier) {
    if (!this->is_modifiable()) {
        return false;
    }
    this->m_assetIdentifier = inAssetIdentifier;
    return true;
};

bool
AssetIdentifier::is_empty() const {
    if (this->m_assetIdentifier.empty() && this->m_resolvedAssetPath.empty()) {
        return true;
    }

    return false;
};

bool
AssetIdentifier::is_valid() const {
    return !this->m_invalidated;
}

void
AssetIdentifier::invalidate() {
    if (!this->is_modifiable()) {
        return;
    };

    this->m_invalidated = true;
};

void
AssetIdentifier::validate() {
    if (!this->is_modifiable()) {
        return;
    };

    this->m_invalidated = false;
};

bool
AssetIdentifier::is_modifiable() const {
    return !this->m_static;
};

void
AssetIdentifier::printInfo() const {
    std::ostringstream oss;
    oss << static_cast<const void*>(this);
    std::cout << "Static; " << this->m_static << " invalidated; " << this->m_invalidated << " AssetIdentifier; "
              << this->m_assetIdentifier << " ResolvedPath; " << this->m_resolvedAssetPath.GetPathString().c_str()
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
