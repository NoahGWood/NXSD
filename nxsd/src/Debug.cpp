#include <nxsd/Debug.h>

#include <iostream>
namespace nxsd {

    void DumpRegistry(const TypeRegistry& reg) {
#ifndef NXSD_DEBUG
        return;
#endif
            std::cout << "=== NXSD TypeRegistry ===\n";
        std::cout << "Type count: " << reg.size() << "\n\n";

        for (const auto& [qname, type] : reg) {
            std::cout << "Type: {" << qname.ns << "}:" << qname.local << "\n";

            if (!type.base_types.empty()) {
                std::cout << "  Bases:\n";
                for (const auto& b : type.base_types) {
                    std::cout << "    - {" << b.ns << "}:" << b.local << "\n";
                }
            }

            if (!type.union_types.empty()) {
                std::cout << "  Union:\n";
                for (const auto& u : type.union_types) {
                    std::cout << "    - {" << u.ns << "}:" << u.local << "\n";
                }
            }

            if (type.primitive.has_value()) {
                std::cout << "  Primitive: "
                          << static_cast<int>(*type.primitive) << "\n";
            }

            if (type.facets.has_value()) {
                const auto& f = *type.facets;
                std::cout << "  Facets:\n";
                if (f.min_inclusive)
                    std::cout << "    minInclusive = " << *f.min_inclusive
                              << "\n";
                if (f.max_inclusive)
                    std::cout << "    maxInclusive = " << *f.max_inclusive
                              << "\n";
                if (f.min_length)
                    std::cout << "    minLength    = " << *f.min_length << "\n";
                if (f.max_length)
                    std::cout << "    maxLength    = " << *f.max_length << "\n";
            }

            if (type.enums.has_value()) {
                std::cout << "  Enum:\n";
                for (const auto& opt : type.enums->options) {
                    std::cout << "    - " << opt << "\n";
                }
            }

            if (!type.elements.empty()) {
                std::cout << "  Elements:\n";
                for (const auto& el : type.elements) {
                    std::cout
                        << "    - " << el.name.local << " : {" << el.type.ns
                        << "}:" << el.type.local << " [" << el.min_occurs << ", "
                        << (el.max_occurs < 0 ? std::string("unbounded")
                                             : std::to_string(el.max_occurs))
                        << "]\n";
                }
            }

            std::cout << "\n";
        }
    }
    void DumpTypeTree(const QName& q, const TypeRegistry& reg,
                      std::unordered_set<QName, QNameHash>& seen, int indent) {
#ifndef NXSD_DEBUG
        return;
#endif
            auto it = reg.find(q);
        if (it == reg.end())
            return;

        const TypeNode& t = it->second;

        std::cout << std::string(indent, ' ') << "- " << q.local << "\n";

        if (!seen.insert(q).second) {
            std::cout << std::string(indent + 2, ' ') << "(cycle)\n";
            return;
        }

        for (const auto& base : t.base_types) {
            std::cout << std::string(indent + 2, ' ') << "extends:\n";
            DumpTypeTree(base, reg, seen, indent + 4);
        }

        for (const auto& el : t.elements) {
            std::cout << std::string(indent + 2, ' ') << "field "
                      << el.name.local << " : " << el.type.local << "\n";
        }
    }
}  // namespace nxsd
