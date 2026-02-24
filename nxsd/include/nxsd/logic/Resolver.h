#pragma once
#include <unordered_set>

#include "../data/Registry.h"

namespace nxsd {
    PrimitiveKind ResolvePrimitive(const QName& type, const TypeRegistry& reg);
    std::optional<ValueFacet> ResolveFacets(const QName& type,
                                            const TypeRegistry& reg);

    // ---- Implementations ---- /
    static PrimitiveKind ResolvePrimitiveImpl(
        const QName& type, const TypeRegistry& reg,
        std::unordered_set<QName, QNameHash>& visiting);

    static void MergeFacet(ValueFacet& dst, const ValueFacet& src);

    static std::optional<ValueFacet> ResolveFacetsImpl(
        const QName& type, const TypeRegistry& reg,
        std::unordered_set<QName, QNameHash>& visited);
}  // namespace nxsd
