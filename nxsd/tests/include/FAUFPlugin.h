#pragma once
#include <optional>
#include <vector>
#include <regex>

#include "Helpers.h"

enum class RECORD_ENCODING {
    TEXT_TAGGED,
    BINARY_TAGGED,
    BINARY_FIXED
};

struct ItemRule {
    QName element;
};

struct SubfieldRule {
    QName element;
    int min_occurs;
    int max_occurs;
    std::vector<ItemRule> items;
};

struct FieldRule {
    QName element;
    std::string ebts_id;  // "2.016"
    int record_type;      // 1, 2, 14
    bool required;
    bool repeatable;

    std::vector<SubfieldRule> subfields;
};

struct RecordRule {
    QName element;
    int record_type;
    RECORD_ENCODING encoding;

    int min_occurs;
    int max_occurs;  // -1 = unbounded

    std::unordered_map<std::string, FieldRule> fields;  // ebts_id -> rule
};

struct TransactionProfile {
    std::string transaction_code;
    std::unordered_map<int, RecordRule> records;  // record_type -> rule
};

struct EbtsFieldAnnotation {
    std::string ebts_id;  // "2.016"
    int record_type;      // 1, 2, 14
    bool optional_hint;
    bool repeatable = false;
    std::vector<QName> path;
    // e.g. [PersonSSNIdentification, IdentificationID]
};
using Annot = AnnotationStore<EbtsFieldAnnotation>;

SchemaRegistry LoadTestSchema();
TransactionProfile CompileTransactionProfile(const SchemaRegistry& schema,
                                             Annot& ebts,
                                             const std::string& tx);

struct ParsedEbtsComment {
    int record_type;
    std::string ebts_id;             // "1.005"
    std::optional<std::string> tag;  // DAT, SOC, etc (optional)
    bool optional_hint = false;
    bool repeatable_hint = false;
};

static std::optional<ParsedEbtsComment> ParseEbtsComment(const std::string& cmt);


void LoadEbtsAnnotations(const SchemaRegistry& schema, Annot& ebts);
void LoadEbtsAnnotationsFromTemplate(
    const SchemaRegistry& schema,
    Annot& ebts,
    const std::filesystem::path& xmlPath);


template<typename Fn>
static void ForEachElement(const SchemaRegistry& schema, Fn&& fn) {
    // Global elements
    for (const auto& [qname, typeQName] : schema.globalElements) {
        fn(qname, typeQName);
    }

    // Elements inside complex types
    for (const auto& [_, type] : schema.types) {
        for (const auto& el : type.elements) {
            fn(el.name, el.type);
        }
    }
}