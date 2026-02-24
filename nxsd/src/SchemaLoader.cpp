#include <nxsd/data/Registry.h>
#include <nxsd/parse/SchemaLoader.h>
#include <tinyxml2.h>

#include <cassert>
#include <cctype>
#include <iostream>
#include <set>
#include <sstream>

namespace nxsd {

    static constexpr const char* XSD_NS = "http://www.w3.org/2001/XMLSchema";
    // Pending comment internals
    struct PendingComments {
        std::vector<std::string> lines;
        void Clear() { lines.clear(); }
        bool Empty() const { return lines.empty(); }
    };

    // ------------------------------
    // Small string helpers
    // ------------------------------
    static std::string ToString(const std::filesystem::path& p) {
        return p.string();
    }

    static std::string GetPrefix(std::string_view qname) {
        auto pos = qname.find(':');
        return (pos == std::string_view::npos)
                   ? ""
                   : std::string(qname.substr(0, pos));
    }
    static std::string GetLocal(std::string_view qname) {
        auto pos = qname.find(':');
        return (pos == std::string_view::npos)
                   ? std::string(qname)
                   : std::string(qname.substr(pos + 1));
    }

    static std::string GetElPrefix(const tinyxml2::XMLElement* el) {
        const char* n = el ? el->Name() : nullptr;
        if (!n)
            return {};
        const char* c = std::strchr(n, ':');
        return c ? std::string(n, c) : std::string{};
    }
    static std::string GetElLocal(const tinyxml2::XMLElement* el) {
        const char* n = el ? el->Name() : nullptr;
        if (!n)
            return {};
        const char* c = std::strchr(n, ':');
        return c ? std::string(c + 1) : std::string(n);
    }

    static std::string GetAttrStr(const tinyxml2::XMLElement* el,
                                  const char* name) {
        if (!el)
            return {};
        const char* v = el->Attribute(name);
        return v ? std::string(v) : std::string{};
    }

    static int ParseOccurs(const std::string& s, int defaultVal) {
        if (s.empty())
            return defaultVal;
        if (s == "unbounded")
            return -1;  // represent unbounded as -1
        try {
            return std::stoi(s);
        } catch (...) {
            return defaultVal;
        }
    }

    static bool IsXsdTag(const tinyxml2::XMLElement* el,
                         const SchemaContext& ctx, const char* localName) {
        if (!el)
            return false;
        const std::string elLocal = GetElLocal(el);
        if (elLocal != localName)
            return false;

        // Verify element's namespace resolves to XSD_NS.
        const std::string pref = GetElPrefix(el);
        std::string ns;
        if (!pref.empty()) {
            auto it = ctx.prefixToNs.find(pref);
            ns = (it != ctx.prefixToNs.end()) ? it->second : std::string{};
        } else {
            // No prefix: use default xmlns mapping if present.
            auto it = ctx.prefixToNs.find("");
            ns = (it != ctx.prefixToNs.end()) ? it->second : std::string{};
        }
        return ns == XSD_NS;
    }

    // ------------------------------
    // QName resolution
    // ------------------------------
    static QName ResolveQName(const SchemaContext& ctx,
                              std::string_view qnameText,
                              bool defaultToTargetNs) {
        QName q{};
        if (qnameText.empty())
            return q;

        const std::string pref = GetPrefix(qnameText);
        const std::string local = GetLocal(qnameText);
        q.local = local;

        if (!pref.empty()) {
            auto it = ctx.prefixToNs.find(pref);
            q.ns = (it != ctx.prefixToNs.end()) ? it->second : std::string{};
        } else {
            // Unprefixed "type/base" generally means targetNamespace for
            // user-defined types. (Built-ins are normally prefixed xs:)
            q.ns = defaultToTargetNs ? ctx.targetNamespace : std::string{};
        }
        return q;
    }

    static QName MakeTargetQName(const SchemaContext& ctx,
                                 const std::string& local) {
        return QName{ctx.targetNamespace, local};
    }

    // ------------------------------
    // Primitive mapping from xs:*
    // ------------------------------
    static std::optional<PrimitiveKind> PrimitiveFromXsd(const QName& q) {
        if (q.ns != XSD_NS)
            return std::nullopt;

        const std::string& t = q.local;
        if (t == "string" || t == "normalizedString" || t == "token")
            return PrimitiveKind::String;
        if (t == "boolean")
            return PrimitiveKind::Boolean;
        if (t == "decimal" || t == "float" || t == "double")
            return PrimitiveKind::Decimal;

        // Integers
        if (t == "integer" || t == "int" || t == "long" || t == "short" ||
            t == "byte" || t == "nonNegativeInteger" ||
            t == "positiveInteger" || t == "nonPositiveInteger" ||
            t == "negativeInteger" || t == "unsignedInt" ||
            t == "unsignedLong" || t == "unsignedShort" || t == "unsignedByte")
            return PrimitiveKind::Integer;

        if (t == "date")
            return PrimitiveKind::Date;
        if (t == "dateTime")
            return PrimitiveKind::DateTime;

        // Binary-ish
        if (t == "base64Binary" || t == "hexBinary")
            return PrimitiveKind::Binary;

        return std::nullopt;
    }

    // ------------------------------
    // Facet parsing helpers
    // ------------------------------
    static void EnsureFacets(TypeNode& t) {
        if (!t.facets.has_value())
            t.facets = ValueFacet{};
    }

    static void ParseRestrictionFacet(TypeNode& t,
                                      const tinyxml2::XMLElement* facetEl,
                                      const std::string& facetName) {
        const std::string val = GetAttrStr(facetEl, "value");
        if (val.empty())
            return;

        // NXSD v1: keep facets as int64 where relevant
        auto to_i64 = [&](const std::string& s, std::optional<int64_t>& out) {
            try {
                out = std::stoll(s);
            } catch (...) {
                // ignore non-integer values in v1
            }
        };

        if (facetName == "minInclusive") {
            EnsureFacets(t);
            to_i64(val, t.facets->min_inclusive);
        } else if (facetName == "maxInclusive") {
            EnsureFacets(t);
            to_i64(val, t.facets->max_inclusive);
        } else if (facetName == "minLength") {
            EnsureFacets(t);
            to_i64(val, t.facets->min_length);
        } else if (facetName == "maxLength") {
            EnsureFacets(t);
            to_i64(val, t.facets->max_length);
        } else if (facetName == "pattern") {
            t.pattern = Pattern{val};
        } else if (facetName == "enumeration") {
            // We'll collect enumerations in EnumSet.options
            if (!t.enums.has_value()) {
                t.enums = EnumSet{};
                t.enums->name = t.name;  // "enum belongs to this type"
            }
            t.enums->options.push_back(val);
        }
    }

    // ------------------------------
    // Inline type naming
    // ------------------------------
    static QName MakeAnonTypeName(const SchemaContext& ctx,
                                  const QName& ownerElementName,
                                  const std::string& suffix) {
        // stable-ish: <ElementLocal>__anon__<suffix>
        std::string local = ownerElementName.local + "__anon__" + suffix;
        return QName{ctx.targetNamespace, local};
    }

    // ------------------------------
    // Context build from xs:schema root
    // ------------------------------
    static SchemaContext BuildContext(const tinyxml2::XMLElement* schemaRoot,
                                      const std::filesystem::path& xsdPath) {
        SchemaContext ctx{};
        ctx.schemaLocationDir = xsdPath.parent_path().string();

        // targetNamespace
        if (schemaRoot) {
            const char* tn = schemaRoot->Attribute("targetNamespace");
            ctx.targetNamespace = tn ? std::string(tn) : std::string{};
            const char* efd = schemaRoot->Attribute("elementFormDefault");
            if (efd && std::string(efd) == "unqualified")
                ctx.elementFormDefaultQualified = false;
        }

        // xmlns mappings are stored as attributes:
        //  xmlns="..."
        //  xmlns:xs="..."
        if (schemaRoot) {
            for (auto* a = schemaRoot->FirstAttribute(); a; a = a->Next()) {
                const char* an = a->Name();
                const char* av = a->Value();
                if (!an || !av)
                    continue;

                std::string name(an);
                if (name == "xmlns") {
                    ctx.prefixToNs[""] = av;
                } else if (name.rfind("xmlns:", 0) == 0) {
                    std::string pref = name.substr(6);
                    ctx.prefixToNs[pref] = av;
                }
            }
        }

        return ctx;
    }

    // ------------------------------
    // PASS A: collect named type shells + simpleType constraints
    // ------------------------------
    static void ParseSimpleTypeDef(const tinyxml2::XMLElement* st,
                                   const SchemaContext& ctx, TypeRegistry& out,
                                   const PendingComments& pending) {
        const std::string name = GetAttrStr(st, "name");
        if (name.empty())
            return;

        TypeNode node{};
        node.name = MakeTargetQName(ctx, name);
        if (!pending.Empty())
            node.comments = CommentBlock{pending.lines};

        // Handle:
        //  <restriction base="..."> facets/enums/pattern
        //  <union memberTypes="..."> or nested <simpleType/>
        for (auto* child = st->FirstChildElement(); child;
             child = child->NextSiblingElement()) {
            if (IsXsdTag(child, ctx, "restriction")) {
                const QName base = ResolveQName(ctx, GetAttrStr(child, "base"),
                                                /*defaultToTargetNs*/ true);
                if (!base.local.empty())
                    node.base_types.push_back(base);

                // Facets
                for (auto* f = child->FirstChildElement(); f;
                     f = f->NextSiblingElement()) {
                    const std::string fLocal = GetElLocal(f);
                    // enumeration/pattern are also facets
                    ParseRestrictionFacet(node, f, fLocal);
                }

                // If base is an XSD builtin, we can set primitive early
                // (resolver can refine/override)
                if (auto pk = PrimitiveFromXsd(base))
                    node.primitive = *pk;

            } else if (IsXsdTag(child, ctx, "union")) {
                // memberTypes="a b c"
                const std::string members = GetAttrStr(child, "memberTypes");
                if (!members.empty()) {
                    std::istringstream iss(members);
                    std::string tok;
                    while (iss >> tok) {
                        node.union_types.push_back(
                            ResolveQName(ctx, tok, true));
                    }
                }
                // nested <simpleType> inside union could exist; we’ll ignore in
                // v1
            }
        }

        // Insert/overwrite
        out[node.name] = std::move(node);
    }

    static void ParseComplexTypeShell(const tinyxml2::XMLElement* ct,
                                      const SchemaContext& ctx,
                                      TypeRegistry& out,
                                      const PendingComments& pending) {
        const std::string name = GetAttrStr(ct, "name");
        if (name.empty())
            return;

        TypeNode node{};
        node.name = MakeTargetQName(ctx, name);
        if (!pending.Empty())
            node.comments = CommentBlock{pending.lines};
        // elements will be filled in Pass B
        out[node.name] = std::move(node);
    }

    // ------------------------------
    // PASS B: fill complex content elements + extensions; handle global
    // elements if desired
    // ------------------------------
    static void ParseSequenceElements(const tinyxml2::XMLElement* seq,
                                      const SchemaContext& ctx,
                                      TypeRegistry& out, TypeNode& ownerType) {
        for (auto* e = seq->FirstChildElement(); e;
             e = e->NextSiblingElement()) {
            if (!IsXsdTag(e, ctx, "element"))
                continue;

            const std::string refAttr = GetAttrStr(e, "ref");
            const std::string nameAttr = GetAttrStr(e, "name");
            const std::string typeAttr = GetAttrStr(e, "type");

            ElementNode en{};
            en.min_occurs = ParseOccurs(GetAttrStr(e, "minOccurs"), 1);
            en.max_occurs = ParseOccurs(GetAttrStr(e, "maxOccurs"), 1);
            // ---- CASE 1: ref= ----
            if (!refAttr.empty()) {
                en.is_ref = true;

                // ref is a QName-valued attribute
                en.name =
                    ResolveQName(ctx, refAttr, /*defaultToTargetNs*/ true);

                // type is resolved later by looking up the global element
                // leave en.type empty for now
            }
            // ---- CASE 2: name= (local or global element declaration) ----
            else if (!nameAttr.empty()) {
                en.is_ref = false;

                // elementFormDefault applies here
                if (ctx.elementFormDefaultQualified) {
                    en.name = QName{ctx.targetNamespace, nameAttr};
                } else {
                    en.name = QName{"", nameAttr};
                }

                if (!typeAttr.empty()) {
                    en.type =
                        ResolveQName(ctx, typeAttr, /*defaultToTargetNs*/ true);
                } else {
                    // inline type:
                    // <element name="X"><simpleType>...</simpleType></element>
                    // We'll generate a named anon type and parse it.
                    for (auto* c = e->FirstChildElement(); c;
                         c = c->NextSiblingElement()) {
                        if (IsXsdTag(c, ctx, "simpleType")) {
                            // Create anon type
                            QName ownerElQ{ctx.targetNamespace,
                                           GetAttrStr(e, "name")};
                            QName anonName =
                                MakeAnonTypeName(ctx, ownerElQ, "simpleType");
                            TypeNode anon{};
                            anon.name = anonName;

                            // Parse inline simpleType like a named one, but
                            // without
                            // @name
                            for (auto* sc = c->FirstChildElement(); sc;
                                 sc = sc->NextSiblingElement()) {
                                if (IsXsdTag(sc, ctx, "restriction")) {
                                    QName base = ResolveQName(
                                        ctx, GetAttrStr(sc, "base"), true);
                                    if (!base.local.empty())
                                        anon.base_types.push_back(base);
                                    for (auto* f = sc->FirstChildElement(); f;
                                         f = f->NextSiblingElement()) {
                                        ParseRestrictionFacet(anon, f,
                                                              GetElLocal(f));
                                    }
                                    if (auto pk = PrimitiveFromXsd(base))
                                        anon.primitive = *pk;
                                } else if (IsXsdTag(sc, ctx, "union")) {
                                    const std::string members =
                                        GetAttrStr(sc, "memberTypes");
                                    if (!members.empty()) {
                                        std::istringstream iss(members);
                                        std::string tok;
                                        while (iss >> tok)
                                            anon.union_types.push_back(
                                                ResolveQName(ctx, tok, true));
                                    }
                                }
                            }

                            out[anon.name] = std::move(anon);
                            en.type = anonName;
                        }
                        // inline complexType support can be added similarly
                    }
                }
            }
            if (IsXsdTag(e, ctx, "choice")) {
                for (auto* c = e->FirstChildElement(); c;
                     c = c->NextSiblingElement()) {
                    if (IsXsdTag(c, ctx, "element")) {
                        // parse exactly like a normal element
                        // ParseOneElement(c, ctx, out, ownerType,
                        //                 /*isChoice=*/true);
                    }
                }
            }

            ownerType.elements.push_back(std::move(en));
        }
    }

    static void ParseComplexTypeBody(const tinyxml2::XMLElement* ct,
                                     const SchemaContext& ctx,
                                     TypeRegistry& out, TypeNode& ownerType) {
        // Support two common forms:
        // 1) <complexType><sequence>...</sequence></complexType>
        // 2) <complexType><complexContent><extension
        // base="..."><sequence>...</sequence></extension></complexContent></complexType>
        for (auto* child = ct->FirstChildElement(); child;
             child = child->NextSiblingElement()) {
            if (IsXsdTag(child, ctx, "sequence")) {
                ParseSequenceElements(child, ctx, out, ownerType);
            } else if (IsXsdTag(child, ctx, "simpleContent")) {
                for (auto* sc = child->FirstChildElement(); sc;
                     sc = sc->NextSiblingElement()) {
                    if (IsXsdTag(sc, ctx, "extension")) {
                        QName base = ResolveQName(ctx, GetAttrStr(sc, "base"),
                                                  /*defaultToTargetNs=*/true);

                        ownerType.base_types.clear();
                        ownerType.base_types.push_back(base);

                        // IMPORTANT: no elements for simpleContent
                        ownerType.elements.clear();
                    }
                }
            } else if (IsXsdTag(child, ctx, "complexContent")) {
                for (auto* cc = child->FirstChildElement(); cc;
                     cc = cc->NextSiblingElement()) {
                    if (IsXsdTag(cc, ctx, "extension")) {
                        QName base =
                            ResolveQName(ctx, GetAttrStr(cc, "base"), true);
                        if (!base.local.empty()) {
                            ownerType.base_types.clear();
                            ownerType.base_types.push_back(base);
                        }
                        // extension may contain sequence
                        for (auto* extChild = cc->FirstChildElement(); extChild;
                             extChild = extChild->NextSiblingElement()) {
                            if (IsXsdTag(extChild, ctx, "sequence")) {
                                ParseSequenceElements(extChild, ctx, out,
                                                      ownerType);
                            }
                        }
                    }
                }
            }
        }
    }

    static void ParseGlobalElement(const tinyxml2::XMLElement* el,
                                   const SchemaContext& ctx,
                                   SchemaRegistry& out,
                                   const PendingComments& pending) {
        const std::string nameAttr = GetAttrStr(el, "name");
        if (nameAttr.empty())
            return;

        QName elemQName{ctx.targetNamespace, nameAttr};

        const std::string typeAttr = GetAttrStr(el, "type");
        if (!typeAttr.empty()) {
            // Normal case: element has explicit type=
            out.globalElements[elemQName] = ResolveQName(ctx, typeAttr, true);
            return;
        }

        // Inline type case (NIEM UserDefinedFields etc.)
        for (auto* c = el->FirstChildElement(); c;
             c = c->NextSiblingElement()) {
            TypeNode t{};

            if (!pending.Empty()) {
                t.comments = CommentBlock{pending.lines};
            }

            if (IsXsdTag(c, ctx, "complexType")) {
                QName anonType{ctx.targetNamespace,
                               nameAttr + "__globalElementType"};

                t.name = anonType;

                ParseComplexTypeBody(c, ctx, out.types, t);
                out.types[anonType] = std::move(t);
                out.globalElements[elemQName] = anonType;
            } else if (IsXsdTag(c, ctx, "simpleType")) {
                QName anonType{ctx.targetNamespace,
                               nameAttr + "__globalElementType"};
                t.name = anonType;

                // Parse like named simpleType
                for (auto* sc = c->FirstChildElement(); sc;
                     sc = sc->NextSiblingElement()) {
                    if (IsXsdTag(sc, ctx, "restriction")) {
                        QName base =
                            ResolveQName(ctx, GetAttrStr(sc, "base"), true);
                        if (!base.local.empty()) {
                            t.base_types.push_back(base);
                        }
                        for (auto* f = sc->FirstChildElement(); f;
                             f = f->NextSiblingElement()) {
                            ParseRestrictionFacet(t, f, GetElLocal(f));
                        }
                        if (auto pk = PrimitiveFromXsd(base)) {
                            t.primitive = *pk;
                        }
                    }
                }

                out.types[anonType] = std::move(t);
                out.globalElements[elemQName] = anonType;
            }
        }
    }

    // ------------------------------
    // Include/import recursion
    // ------------------------------
    struct LoaderState {
        std::set<std::string> visited;  // canonical path strings
    };

    static bool LoadOneFileInternal(const std::filesystem::path& xsd,
                                    SchemaRegistry& out,
                                    const ParseOptions& opts, LoaderState& st);

    static bool ParseFilePasses(const std::filesystem::path& xsd,
                                SchemaRegistry& out, const ParseOptions& opts,
                                LoaderState& st) {
        tinyxml2::XMLDocument doc;
        const auto xsdStr = ToString(xsd);
        if (doc.LoadFile(xsdStr.c_str()) != tinyxml2::XML_SUCCESS) {
            std::cerr << "NXSD: failed to load XSD: " << xsdStr << "\n";
            return false;
        }

        auto* schemaRoot = doc.RootElement();
        if (!schemaRoot)
            return false;

        SchemaContext ctx = BuildContext(schemaRoot, xsd);

        // Follow includes/imports first (so referenced types exist)
        if (opts.follow_includes || opts.follow_imports) {
            for (auto* el = schemaRoot->FirstChildElement(); el;
                 el = el->NextSiblingElement()) {
                if (opts.follow_includes && IsXsdTag(el, ctx, "include")) {
                    const std::string loc = GetAttrStr(el, "schemaLocation");
                    if (!loc.empty()) {
                        std::filesystem::path inc =
                            std::filesystem::path(ctx.schemaLocationDir) / loc;
                        if (!LoadOneFileInternal(inc, out, opts, st))
                            return false;
                    }
                }
                if (opts.follow_imports && IsXsdTag(el, ctx, "import")) {
                    const std::string loc = GetAttrStr(el, "schemaLocation");
                    if (!loc.empty()) {
                        std::filesystem::path imp =
                            std::filesystem::path(ctx.schemaLocationDir) / loc;
                        if (!LoadOneFileInternal(imp, out, opts, st))
                            return false;
                    }
                    // Note: import without schemaLocation is common; v1 ignores
                    // it.
                }
            }
        }

        PendingComments pending;

        // ---------------- PASS A ----------------
        for (auto* n = schemaRoot->FirstChild(); n; n = n->NextSibling()) {
            if (auto* c = n->ToComment()) {
                pending.lines.emplace_back(c->Value());
                continue;
            }
            auto* el = n->ToElement();
            if (!el) {
                continue;
            }
            if (IsXsdTag(el, ctx, "simpleType")) {
                ParseSimpleTypeDef(el, ctx, out.types, pending);
                pending.Clear();
            } else if (IsXsdTag(el, ctx, "complexType")) {
                ParseComplexTypeShell(el, ctx, out.types, pending);
                pending.Clear();
            } else if (IsXsdTag(el, ctx, "element")) {
                ParseGlobalElement(el, ctx, out, pending);
                pending.Clear();
            }
        }
        /*
        for (auto* el = schemaRoot->FirstChildElement(); el;
             el = el->NextSiblingElement()) {
            if (IsXsdTag(el, ctx, "simpleType")) {
                ParseSimpleTypeDef(el, ctx, out.types);
            } else if (IsXsdTag(el, ctx, "complexType")) {
                ParseComplexTypeShell(el, ctx, out.types);
            }
            if (IsXsdTag(el, ctx, "element")) {
                const std::string nameAttr = GetAttrStr(el, "name");
                if (nameAttr.empty())
                    continue;

                QName elemQName{ctx.targetNamespace, nameAttr};

                const std::string typeAttr = GetAttrStr(el, "type");
                if (!typeAttr.empty()) {
                    // Normal case: element has explicit type=
                    out.globalElements[elemQName] =
                        ResolveQName(ctx, typeAttr, true);
                    continue;
                }

                // Inline type case (NIEM UserDefinedFields etc.)
                for (auto* c = el->FirstChildElement(); c;
                     c = c->NextSiblingElement()) {
                    if (IsXsdTag(c, ctx, "complexType")) {
                        QName anonType{ctx.targetNamespace,
                                       nameAttr + "__globalElementType"};

                        TypeNode t{};
                        t.name = anonType;

                        ParseComplexTypeBody(c, ctx, out.types, t);
                        out.types[anonType] = std::move(t);

                        out.globalElements[elemQName] = anonType;
                    } else if (IsXsdTag(c, ctx, "simpleType")) {
                        QName anonType{ctx.targetNamespace,
                                       nameAttr + "__globalElementType"};

                        TypeNode t{};
                        t.name = anonType;

                        // Parse like named simpleType
                        for (auto* sc = c->FirstChildElement(); sc;
                             sc = sc->NextSiblingElement()) {
                            if (IsXsdTag(sc, ctx, "restriction")) {
                                QName base = ResolveQName(
                                    ctx, GetAttrStr(sc, "base"), true);
                                if (!base.local.empty())
                                    t.base_types.push_back(base);
                                for (auto* f = sc->FirstChildElement(); f;
                                     f = f->NextSiblingElement()) {
                                    ParseRestrictionFacet(t, f, GetElLocal(f));
                                }
                                if (auto pk = PrimitiveFromXsd(base))
                                    t.primitive = *pk;
                            }
                        }

                        out.types[anonType] = std::move(t);
                        out.globalElements[elemQName] = anonType;
                    }
                }
            }
        }
            */

        // ---------------- PASS B ----------------
        for (auto* el = schemaRoot->FirstChildElement(); el;
             el = el->NextSiblingElement()) {
            if (!IsXsdTag(el, ctx, "complexType"))
                continue;

            const std::string name = GetAttrStr(el, "name");
            if (name.empty())
                continue;
            QName qn = MakeTargetQName(ctx, name);

            auto it = out.types.find(qn);
            if (it == out.types.end())
                continue;

            // clear and refill elements in case of reload/overwrite
            it->second.elements.clear();

            ParseComplexTypeBody(el, ctx, out.types, it->second);
        }

        return true;
    }

    static bool LoadOneFileInternal(const std::filesystem::path& xsd,
                                    SchemaRegistry& out,
                                    const ParseOptions& opts, LoaderState& st) {
        std::error_code ec;
        auto canon = std::filesystem::weakly_canonical(xsd, ec);
        const std::string key =
            ec ? ToString(std::filesystem::absolute(xsd)) : ToString(canon);

        if (st.visited.contains(key))
            return true;  // already loaded

        st.visited.insert(key);

        if (!std::filesystem::exists(xsd)) {
            std::cerr << "NXSD: missing XSD: " << ToString(xsd) << "\n";
            return false;
        }

        return ParseFilePasses(xsd, out, opts, st);
    }

    // ------------------------------
    // Public API
    // ------------------------------
    bool SchemaLoader::LoadDirectory(const std::filesystem::path& dir,
                                     SchemaRegistry& out) {
        if (!std::filesystem::exists(dir) ||
            !std::filesystem::is_directory(dir))
            return false;

        LoaderState st;
        for (auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file())
                continue;
            if (entry.path().extension() != ".xsd")
                continue;

            if (!LoadOneFileInternal(entry.path(), out, opts, st))
                return false;
        }
        return true;
    }

    bool SchemaLoader::LoadFile(const std::filesystem::path& xsd,
                                SchemaRegistry& out) {
        LoaderState st;
        bool success = LoadOneFileInternal(xsd, out, opts, st);
        ResolveSimpleContentWrappers(out);
        ResolveElementRefs(out);
        return success;
    }

    // ResolvedType SchemaLoader::Resolve(const QName& type){
    //     ResolveType type;

    //     return type;
    // }
    void ResolveElementRefs(SchemaRegistry& reg) {
        for (auto& [_, type] : reg.types) {
            for (auto& el : type.elements) {
                if (!el.is_ref)
                    continue;

                auto it = reg.globalElements.find(el.name);
                if (it != reg.globalElements.end()) {
                    el.type = it->second;
                }
            }
        }
    }

    void ResolveSimpleContentWrappers(SchemaRegistry& reg) {
        for (auto& [_, type] : reg.types) {
            if (type.elements.empty() && type.base_types.size() == 1) {
                const QName& base = type.base_types[0];
                auto it = reg.types.find(base);
                if (it == reg.types.end())
                    continue;

                const TypeNode& baseType = it->second;

                // SimpleContent wrapper case
                if (baseType.primitive.has_value() ||
                    baseType.enums.has_value()) {
                    type.primitive = baseType.primitive;
                    type.enums = baseType.enums;
                    type.facets = baseType.facets;
                }
            }
        }
    }

}  // namespace nxsd
