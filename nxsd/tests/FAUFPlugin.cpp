#include <tinyxml2.h>

#include <filesystem>
#include <iostream>

#include "FAUFPlugin.h"

SchemaRegistry LoadTestSchema() {
    std::filesystem::path testDir = NXSD_TEST_DATA_DIR;
    std::filesystem::path mpdPath =
        testDir / "EBTS 11.2 XML IEPD_20250212/mpd-catalog.xml";

    // ---- Step 1: Discover schema paths from MPD ----
    std::vector<std::filesystem::path> schemas;

    {
        tinyxml2::XMLDocument doc;
        doc.LoadFile(mpdPath.string().c_str());

        auto* root = doc.RootElement();

        for (auto* el = root->FirstChildElement(); el;
             el = el->NextSiblingElement()) {
            // Walk descendants looking for c:XMLSchemaDocument /
            // c:ExtensionSchemaDocument
            std::function<void(tinyxml2::XMLElement*)> walk =
                [&](tinyxml2::XMLElement* n) {
                    if (!n)
                        return;

                    std::string local = n->Name();
                    auto pos = local.find(':');
                    if (pos != std::string::npos)
                        local = local.substr(pos + 1);

                    if (local == "XMLSchemaDocument" ||
                        local == "ExtensionSchemaDocument") {
                        const char* path = n->Attribute("c:pathURI");
                        if (path) {
                            schemas.push_back(mpdPath.parent_path() / path);
                        }
                    }

                    for (auto* c = n->FirstChildElement(); c;
                         c = c->NextSiblingElement())
                        walk(c);
                };

            walk(el);
        }
    }
    SchemaLoader loader;
    SchemaRegistry reg;
    for (const auto& xsd : schemas) {
        loader.LoadFile(xsd, reg);
    }
    return reg;
}

static std::optional<ParsedEbtsComment> ParseEbtsComment(
    const std::string& cmt) {
    // Normalize whitespace
    std::string s = cmt;
    std::replace(s.begin(), s.end(), '\n', ' ');

    // Examples:
    // "DAT 1.005"
    // "SOC 2.016 (0..4)"

    std::regex r(R"(([A-Z]{2,3})?\s*([0-9]+)\.([0-9]{3}))");
    std::smatch m;
    if (!std::regex_search(s, m, r))
        return std::nullopt;

    ParsedEbtsComment out{};
    out.tag = m[1].matched ? std::optional<std::string>(m[1]) : std::nullopt;
    out.record_type = std::stoi(m[2]);
    out.ebts_id = m[2].str() + "." + m[3].str();
    // Optional hints:
    // - literal "Optional"
    // - or a range that starts at 0 like "(0..4)" / "(0..n)"
    static const std::regex opt_word(R"(\bOptional\b)", std::regex::icase);
    static const std::regex range_re(R"(\(\s*(\d+)\s*\.\.\s*([0-9nN]+)\s*\))");

    if (std::regex_search(cmt, opt_word))
        out.optional_hint = true;

    std::smatch rm;
    if (std::regex_search(cmt, rm, range_re)) {
        int lo = 1;
        try {
            lo = std::stoi(rm[1].str());
        } catch (...) {
        }
        if (lo == 0)
            out.optional_hint = true;

        // repeatable if upper bound != 1 (treat n as repeatable)
        const std::string hi = rm[2].str();
        if (hi == "n" || hi == "N")
            out.repeatable_hint = true;
        else {
            try {
                out.repeatable_hint = (std::stoi(hi) != 1);
            } catch (...) {
            }
        }
    }

    return out;
}

void LoadEbtsAnnotations(const SchemaRegistry& schema, Annot& ebts) {
    for (const auto& [qname, element] : schema.globalElements) {
        SchemaNodeRef ref{SCHEMA_NODE_KIND::ELEMENT, qname};
        // Example mappings (adjust to real element names)
        // std::cout << "Name: " << qname.local << "\n";
        if (qname.local == "TransactionDate") {
            ebts.Set(ref, EbtsFieldAnnotation{.ebts_id = "1.005",
                                              .record_type = 1,
                                              .optional_hint = false});
        } else if (qname.local == "PersonSSNIdentification") {
            ebts.Set(ref, EbtsFieldAnnotation{.ebts_id = "2.016",
                                              .record_type = 2,
                                              .optional_hint = false});
        }

        // Add more FAUF mappings here
    }
}

const ElementNode* FindElementInType(const SchemaRegistry& schema,
                                     const QName& typeName,
                                     const QName& elementName) {
    auto itType = schema.types.find(typeName);
    if (itType == schema.types.end())
        return nullptr;

    const TypeNode& type = itType->second;

    for (const auto& el : type.elements) {
        if (el.name == elementName)
            return &el;
    }
    return nullptr;
}
static bool IsSimpleValueType(const SchemaRegistry& schema,
                              const QName& typeQName) {
    auto it = schema.types.find(typeQName);
    if (it == schema.types.end())
        return false;

    const TypeNode& t = it->second;

    // Direct primitive or enum
    if (t.primitive.has_value() || t.enums.has_value())
        return true;

    // SimpleContent wrapper: base type carries primitive
    if (t.elements.empty() && t.base_types.size() == 1) {
        return IsSimpleValueType(schema, t.base_types[0]);
    }

    return false;
}

#include <tinyxml2.h>
#include <unordered_map>

static QName ResolveTemplateQName(
    const tinyxml2::XMLElement* el,
    const std::unordered_map<std::string, std::string>& prefixToNs)
{
    const char* name = el->Name(); // e.g. "nc:PersonSSNIdentification"
    std::string full = name ? name : "";

    auto pos = full.find(':');
    if (pos == std::string::npos) {
        return QName{"", full};
    }

    std::string prefix = full.substr(0, pos);
    std::string local  = full.substr(pos + 1);

    auto it = prefixToNs.find(prefix);
    std::string ns = (it != prefixToNs.end()) ? it->second : "";

    return QName{ns, local};
}

void LoadEbtsAnnotationsFromTemplate(
    const SchemaRegistry& schema,
    Annot& ebts,
    const std::filesystem::path& xmlPath)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(xmlPath.string().c_str()) != tinyxml2::XML_SUCCESS) {
        return;
    }

    auto* root = doc.RootElement();
    if (!root) return;

    // ---- collect xmlns mappings from root ----
    std::unordered_map<std::string, std::string> prefixToNs;
    for (auto* a = root->FirstAttribute(); a; a = a->Next()) {
        std::string name = a->Name();
        if (name == "xmlns") {
            prefixToNs[""] = a->Value();
        } else if (name.rfind("xmlns:", 0) == 0) {
            prefixToNs[name.substr(6)] = a->Value();
        }
    }

    std::vector<std::string> pendingComments;

    std::function<void(tinyxml2::XMLNode*)> walk =
        [&](tinyxml2::XMLNode* n) {
            for (auto* c = n->FirstChild(); c; c = c->NextSibling()) {

                if (auto* com = c->ToComment()) {
                    pendingComments.emplace_back(com->Value());
                    continue;
                }

                if (auto* el = c->ToElement()) {
                    // Try to parse EBTS comment
                    std::optional<ParsedEbtsComment> parsed;
                    for (const auto& line : pendingComments) {
                        parsed = ParseEbtsComment(line);
                        if (parsed) break;
                    }

                    if (parsed) {
                        QName q =
                            ResolveTemplateQName(el, prefixToNs);

                        SchemaNodeRef ref{
                            SCHEMA_NODE_KIND::ELEMENT,
                            q
                        };

                        EbtsFieldAnnotation ann{};
                        ann.ebts_id        = parsed->ebts_id;
                        ann.record_type    = parsed->record_type;
                        ann.optional_hint  = parsed->optional_hint;
                        ann.repeatable     = parsed->repeatable_hint;

                        ebts.Set(ref, std::move(ann));
                    }

                    pendingComments.clear();
                }

                walk(c);
            }
        };

    walk(root);
}


TransactionProfile CompileTransactionProfile(const SchemaRegistry& schema,
                                             Annot& ebts,
                                             const std::string& txn) {
    TransactionProfile profile;
    profile.transaction_code = txn;

    // for (const auto& [qname, element] : schema.globalElements) {

    //     SchemaNodeRef ref {
    //         SCHEMA_NODE_KIND::ELEMENT,
    //         qname
    //     };

    //     const auto* ann = ebts.Get(ref);
    //     if (!ann)
    //         continue;

    //     FieldRule rule;
    //     rule.element = qname;
    //     rule.ebts_id = ann->ebts_id;
    //     rule.record_type = ann->record_type;

    //     rule.required =
    //         (element.min_occurs > 0) &&
    //         !ann->optional_hint;

    //     rule.repeatable =
    //         (element.max_occurs != 1);

    //     auto& record = profile.records[ann->record_type];
    //     record.record_type = ann->record_type;
    //     record.fields[ann->ebts_id] = rule;
    // }
    for (const auto& [typeName, typeNode] : schema.types) {
        for (const auto& element : typeNode.elements) {
            SchemaNodeRef ref{SCHEMA_NODE_KIND::ELEMENT, element.name};

            const auto* ann = ebts.Get(ref);
            if (!ann)
                continue;

            FieldRule rule;
            rule.element = element.name;
            rule.ebts_id = ann->ebts_id;
            rule.record_type = ann->record_type;

            rule.required = (element.min_occurs > 0) && !ann->optional_hint;

            rule.repeatable = (element.max_occurs != 1);

            auto& record = profile.records[ann->record_type];
            record.record_type = ann->record_type;
            record.fields[ann->ebts_id] = rule;
        }
    }

    // --- FAUF-specific record cardinality ---
    if (txn == "FAUF") {
        profile.records[1].min_occurs = 1;
        profile.records[1].max_occurs = 1;

        profile.records[2].min_occurs = 1;
        profile.records[2].max_occurs = 1;

        profile.records[14].min_occurs = 1;
        profile.records[14].max_occurs = 10;
    }

    return profile;
}
