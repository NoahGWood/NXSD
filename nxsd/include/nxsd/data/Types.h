#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nxsd {

    enum class PrimitiveKind {
        String,
        Integer,
        Decimal,
        Boolean,
        Date,
        DateTime,
        Binary,
        Unknown
    };

    struct QName {
        std::string ns;
        std::string local;
    };

    struct CommentBlock {
        std::vector<std::string> lines;  // Preserve raw text lines
    };

    struct ElementNode {
        QName name;
        QName type;
        int min_occurs;
        int max_occurs;

        bool is_ref = false;
        bool is_choice = false;
        bool IsOptional() const { return min_occurs == 0; }
        bool IsRepeated() const { return max_occurs != 1; }

        std::optional<CommentBlock> comments;
    };

    struct ValueFacet {
        std::optional<int64_t> min_inclusive;
        std::optional<int64_t> max_inclusive;
        std::optional<int64_t> min_length;
        std::optional<int64_t> max_length;
    };
    struct EnumSet {
        QName name;
        std::vector<std::string> options;
    };
    struct Pattern {
        std::string regex;
    };

    struct TypeNode {
        QName name;

        // Structure
        std::vector<ElementNode> elements;  // If complex types
        // base_types[0] is the immediate base type (restriction or extension)
        // additional entries only appear during resolution (flattened)
        std::vector<QName> base_types;  // Restriction/extensions
        std::vector<QName> union_types;

        // Constraints
        std::optional<ValueFacet> facets;
        std::optional<EnumSet> enums;
        std::optional<Pattern> pattern;
        std::optional<PrimitiveKind> primitive;

        std::optional<CommentBlock> comments;

        bool resolved = false;

        bool IsSimple() const { return elements.empty(); }

        bool IsComplex() const { return !elements.empty(); }
    };

    struct ResolvedType {
        PrimitiveKind primitive;
        std::optional<ValueFacet> facets;
        std::optional<EnumSet> enums;
        bool is_union;
        bool is_complex;
    };

}  // namespace nxsd
