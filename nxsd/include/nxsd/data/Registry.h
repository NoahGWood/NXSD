#pragma once
#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>

#include "Types.h"

namespace nxsd {

    inline bool operator==(const QName& a, const QName& b) noexcept {
        return a.ns == b.ns && a.local == b.local;
    }

    struct QNameHash {
        size_t operator()(const QName& q) const noexcept {
            // stable combine
            size_t h1 = std::hash<std::string>()(q.ns);
            size_t h2 = std::hash<std::string>()(q.local);
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };

    using TypeRegistry = std::unordered_map<QName, TypeNode, QNameHash>;
    using GlobalElementMap = std::unordered_map<QName, QName, QNameHash>;

    struct SchemaRegistry {
        TypeRegistry types;
        GlobalElementMap globalElements;
    };
}  // namespace nxsd
