#include "assetIdentDef.h"
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

ArResolvedPath
assetIdent::getResolvedAssetPath() const {
    return m_resolvedAssetPath;
};

bool
assetIdent::setResolvedAssetPath(const ArResolvedPath &inResolvedAssetPath) {
    if (!this->is_modifiable()) {
        return false;
    }
    this->m_resolvedAssetPath = inResolvedAssetPath;
    return true;
};
bool
assetIdent::setResolvedAssetPath(const std::string &inResolvedAssetPath) {
    if (!this->is_modifiable()) {
        return false;
    }
    this->m_resolvedAssetPath = ArResolvedPath(inResolvedAssetPath);
    return true;
};

std::string
assetIdent::getAssetIdentifier() const {
    return m_assetIdentifier;
};

bool
assetIdent::setAssetIdentifier(const std::string inAssetIdentifier) {
    if (!this->is_modifiable()) {
        return false;
    }
    this->m_assetIdentifier = inAssetIdentifier;
    return true;
};

bool
assetIdent::is_empty() const {
    if (this->m_assetIdentifier.empty() && this->m_resolvedAssetPath.empty()) {
        return true;
    }

    return false;
};

bool
assetIdent::is_valid() const {
    return !this->m_invalidated;
}

void
assetIdent::invalidate() {
    if (!this->is_modifiable()) {
        return;
    };

    this->m_invalidated = true;
};

void
assetIdent::validate() {
    if (!this->is_modifiable()) {
        return;
    };

    this->m_invalidated = false;
};

bool
assetIdent::is_modifiable() const {
    return !this->m_static;
};

void
assetIdent::printInfo() const {
    std::ostringstream oss;
    oss << static_cast<const void*>(this);
    std::cout << "Static; " << this->m_static << " invalidated; " << this->m_invalidated << " AssetIdentifier; "
              << this->m_assetIdentifier << " ResolvedPath; " << this->m_resolvedAssetPath.GetPathString().c_str()
              << " Instance_m_Pose; " << oss.str().c_str() << " Instance_m_Size; "
              << std::to_string(sizeof(*this)).c_str() << std::endl;
};

bool
assetIdent::operator==(const assetIdent &other) const {
    return m_assetIdentifier == other.m_assetIdentifier;
};

assetIdent &
assetIdent::operator=(const assetIdent &other) {
    if (this != &other) {
        m_resolvedAssetPath = other.m_resolvedAssetPath;
        m_assetIdentifier = other.m_assetIdentifier;
        m_invalidated = other.m_invalidated;
    }
    return *this;
}
