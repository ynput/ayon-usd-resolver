#ifndef ASSET_IDENTIFIER_DEF
#define ASSET_IDENTIFIER_DEF

#include <string>
#include "pxr/pxr.h"
#include "pxr/usd/ar/resolvedPath.h"

/**
 * @brief cache element class used to represent an Usd asset in cache for the resolver
 *
 * @param is_static
 * @return
 */
PXR_NAMESPACE_USING_DIRECTIVE

class AssetIdentifier {
    public:
        AssetIdentifier(): m_static(false) {
        }
        AssetIdentifier(bool is_static): m_static(is_static) {
        }
        AssetIdentifier(const std::string &assetIdentifier): m_assetIdentifier(assetIdentifier), m_static(false) {
        }
        AssetIdentifier(const ArResolvedPath &path, const std::string &identifier, bool is_static):
            m_resolvedAssetPath(path),
            m_assetIdentifier(identifier),
            m_static(is_static) {
        }

        /**
         * @brief returns the ResolvedAssetPath for this AssetIdentifier
         *
         * @return Pxr ArResolvedPath
         */
        ArResolvedPath getResolvedAssetPath() const;
        /**
         * @brief allows setting the ResolvedAssetPath
         *
         * @param inResolvedAssetPath
         * @return false if AssetIdentifier can't be modified (eg. is_valid)
         */
        bool setResolvedAssetPath(const ArResolvedPath &inResolvedAssetPath);
        bool setResolvedAssetPath(const std::string &inResolvedAssetPath);

        /**
         * @brief returns the asset identifier
         */
        std::string getAssetIdentifier() const;

        /**
         * @brief allows you to set the internal m_assetIdentifier
         *
         * @param inAssetIdentifier
         * @return
         */
        bool setAssetIdentifier(const std::string &inAssetIdentifier);

        /**
         * @brief can be used to know if given AssetIdentifier has data in it
         *
         * @return bool: true if neither m_resolvedAssetPath or m_assetIdentifier have any data in them
         */
        bool is_empty() const;
        /**
         * @brief this function allows you to know if an given AssetIdentifier's cache is still valid. This can be use full
         * for TTL or cache invalidation as we use lazy reaching for cached objects
         *
         * @return true if the current data can safely be used. false if the data is out of data or should in  be
         * re-cached
         */
        bool is_valid() const;

        /**
         * @brief allows you to invalidate this AssetIdentifier. It is not possible to invalidate an AssetIdentifier that is not
         * modifiable in this case the function will simply return
         */
        void invalidate();

        /**
         * @brief allows you to validate this AssetIdentifier. This function will return while doing nothing if is_modifiable
         * returns false
         */
        void validate();

        /**
         * @brief allows you to know if you can modify the data in this AssetIdentifier it is also used in getAssetIdentifier
         * and getResolvedAssetPath to keep you from modifying data you should not touch. This will be the case if this
         * AssetIdentifier is marked as static
         *
         * @return true if you are allowed to modify the AssetIdentifier, false if the modification is not allowed for what
         * ever reason
         */
        bool is_modifiable() const;

        /**
         * @brief this function will print out all debug info about this AssetIdentifier instance
         */
        void printInfo() const;

        bool operator==(const AssetIdentifier &other) const;

        AssetIdentifier &operator=(const AssetIdentifier &other);

    private:
        const bool m_static;
        bool m_invalidated;
        ArResolvedPath m_resolvedAssetPath;
        std::string m_assetIdentifier;
};

struct AssetIdentifierHash {
        size_t
        operator()(const AssetIdentifier &instance) const {
            return std::hash<std::string>()(instance.getAssetIdentifier());
        }

        size_t
        operator()(const std::string &str) const {
            return std::hash<std::string>()(str);
        }
};

#endif   // !ASSET_IDENTIFIER_DEF
