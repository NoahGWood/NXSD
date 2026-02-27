#pragma once
#include <unordered_set>

#include "../data/Registry.h"

namespace nxsd {

    ResolvedType ResolveType(const QName& type, const SchemaRegistry& reg);
    PrimitiveKind ResolvePrimitive(const QName& type, const TypeRegistry& reg);
    std::optional<ValueFacet> ResolveFacets(const QName& type,
                                            const TypeRegistry& reg);

    // ---- Implementations ---- /
    PrimitiveKind ResolvePrimitiveImpl(
        const QName& type, const TypeRegistry& reg,
        std::unordered_set<QName, QNameHash>& visiting);

    void MergeFacet(ValueFacet& dst, const ValueFacet& src);

    std::optional<ValueFacet> ResolveFacetsImpl(
        const QName& type, const TypeRegistry& reg,
        std::unordered_set<QName, QNameHash>& visited);
}  // namespace nxsd
