#pragma once
#include <nxsd/data/Registry.h>
#include <nxsd/data/Types.h>

namespace nxsd {

    enum class SCHEMA_NODE_KIND {
        ELEMENT,
        TYPE
    };

    struct SchemaNodeRef {
        SCHEMA_NODE_KIND kind;
        QName name;

        bool operator==(const SchemaNodeRef& other) const {
            return kind == other.kind && name.ns == other.name.ns &&
                   name.local == other.name.local;
        }
    };

    struct SchemaNodeRefHash {
        std::size_t operator()(const SchemaNodeRef& k) const noexcept {
            std::size_t h1 = std::hash<std::string>{}(k.name.ns);
            std::size_t h2 = std::hash<std::string>{}(k.name.local);
            std::size_t h3 = std::hash<int>{}(static_cast<int>(k.kind));

            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    template <typename T>
    class AnnotationStore {
      public:
        template <typename U>
        void Set(const SchemaNodeRef& ref, U&& value) {
            m_Map[ref] = std::forward<U>(value);
        }

        const T* Get(const SchemaNodeRef& ref) const {
            auto it = m_Map.find(ref);
            return it != m_Map.end() ? &it->second : nullptr;
        }
        // 🔴 DEBUG / INTROSPECTION
        template <typename Fn>
        void ForEach(Fn&& fn) const {
            for (const auto& [ref, value] : m_Map) {
                fn(ref, value);
            }
        }

      private:
        std::unordered_map<SchemaNodeRef, T, SchemaNodeRefHash> m_Map;
    };

    template <typename T>
    struct IAnnotation {
        virtual ~IAnnotation() = default;
        virtual void Annotate(const SchemaRegistry& schema,
                              AnnotationStore<T>& store) = 0;
    };

    /**
     * @brief Use this to create annotations ala:
     *
     * EBTS Annotation:
     * SchemaNodeRef ref {
     *      .kind = SchemaNodeKind::Element,
     *      .name = element_qname
     *  };
     *
     *  store.by_node[ref].push_back(EbtsFieldAnnotation{
     *      .ebts_id = "2.016",
     *      .record_type = 2,
     *      .tag = "DAT"
     *  });
     *
     * Type-level enum annotation:
     * SchemaNodeRef ref {
     *     .kind = SchemaNodeKind::Type,
     *     .name = type_qname
     * };
     *
     * store.by_node[ref].push_back(EnumAnnotation{...});
     */

}  // namespace nxsd
