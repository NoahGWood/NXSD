#include <nxsd/logic/Resolver.h>

#include <stdexcept>

namespace nxsd {

    PrimitiveKind ResolvePrimitive(const QName& type, const TypeRegistry& reg) {
        std::unordered_set<QName, QNameHash> visiting;
        visiting.reserve(256);
        return ResolvePrimitiveImpl(type, reg, visiting);
    }

    std::optional<ValueFacet> ResolveFacets(const QName& type,
                                            const TypeRegistry& reg) {
        std::unordered_set<QName, QNameHash> visited;
        return ResolveFacetsImpl(type, reg, visited);
    }

    PrimitiveKind ResolvePrimitiveImpl(
        const QName& type, const TypeRegistry& reg,
        std::unordered_set<QName, QNameHash>& visiting) {
        // insert returns {iterator, inserted}
        auto [_, inserted] = visiting.insert(type);
        if (!inserted) {
            // already in current DFS stack => cycle
            return PrimitiveKind::Unknown;
        }

        auto it = reg.find(type);
        if (it == reg.end())
            return PrimitiveKind::Unknown;

        const TypeNode& node = it->second;

        if (node.primitive.has_value())
            return *node.primitive;

        // Complex types: no primitive
        if (!node.elements.empty())
            return PrimitiveKind::Unknown;

        for (const auto& base : node.base_types) {
            PrimitiveKind pk = ResolvePrimitiveImpl(base, reg, visiting);
            if (pk != PrimitiveKind::Unknown)
                return pk;
        }

        // unions: return first non-unknown (optional; safe default)
        for (const auto& u : node.union_types) {
            PrimitiveKind pk = ResolvePrimitiveImpl(u, reg, visiting);
            if (pk != PrimitiveKind::Unknown)
                return pk;
        }

        return PrimitiveKind::Unknown;
    }

    void MergeFacet(ValueFacet& dst, const ValueFacet& src) {
        if (src.min_inclusive)
            dst.min_inclusive = dst.min_inclusive ? std::max(*dst.min_inclusive,
                                                             *src.min_inclusive)
                                                  : src.min_inclusive;

        if (src.max_inclusive)
            dst.max_inclusive = dst.max_inclusive ? std::min(*dst.max_inclusive,
                                                             *src.max_inclusive)
                                                  : src.max_inclusive;

        if (src.min_length)
            dst.min_length = dst.min_length
                                 ? std::max(*dst.min_length, *src.min_length)
                                 : src.min_length;

        if (src.max_length)
            dst.max_length = dst.max_length
                                 ? std::min(*dst.max_length, *src.max_length)
                                 : src.max_length;
        if (dst.min_inclusive && dst.max_inclusive &&
            *dst.min_inclusive > *dst.max_inclusive) {
            throw std::logic_error("Invalid facet merge: min > max");
        }
    }
    std::optional<ValueFacet> ResolveFacetsImpl(
        const QName& type, const TypeRegistry& reg,
        std::unordered_set<QName, QNameHash>& visited) {
        if (visited.contains(type))
            return std::nullopt;

        visited.insert(type);

        auto it = reg.find(type);
        if (it == reg.end())
            return std::nullopt;

        const TypeNode& node = it->second;

        std::optional<ValueFacet> result;

        // 1. Resolve base facets first
        for (const auto& base : node.base_types) {
            if (auto bf = ResolveFacetsImpl(base, reg, visited)) {
                if (!result)
                    result = ValueFacet{};
                MergeFacet(*result, *bf);
            }
        }

        // 2. Overlay this node's facets
        if (node.facets) {
            if (!result)
                result = ValueFacet{};
            MergeFacet(*result, *node.facets);
        }

        return result;
    }

    nxsd::ResolvedType ResolveType(const nxsd::QName& type,
                                   const nxsd::SchemaRegistry& reg) {
        if (auto it = reg.resolved_cache.find(type);
            it != reg.resolved_cache.end()) {
            return it->second;
        }
        
        nxsd::ResolvedType out{};
        out.is_union = false;
        out.is_complex = false;

        auto it = reg.types.find(type);
        if (it == reg.types.end()) {
            out.primitive = nxsd::PrimitiveKind::Unknown;
            return out;
        }

        const nxsd::TypeNode& node = it->second;

        if (node.IsComplex()) {
            out.is_complex = true;
            out.primitive = nxsd::PrimitiveKind::Unknown;
            return out;
        }

        // ---- primitive ----
        out.primitive = ResolvePrimitive(type, reg.types);

        // ---- facets ----
        out.facets = ResolveFacets(type, reg.types);

        // ---- enums ----
        if (node.enums)
            out.enums = node.enums;

        // ---- union ----
        if (!node.union_types.empty()) {
            out.is_union = true;
            out.primitive = nxsd::PrimitiveKind::String;  // safest fallback
        }

        return out;
    }
}  // namespace nxsd
