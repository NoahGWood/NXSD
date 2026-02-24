#pragma once
#include <nxsd/nxsd.h>
#include <tinyxml2.h>

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace iep {

    struct CommentBlock {
        std::vector<std::string> lines;
    };

    struct ElementVisit {
        nxsd::QName name;
        CommentBlock comments;
        const tinyxml2::XMLElement* xml;  // optional escape hatch
    };

    struct WalkContext {
        std::vector<nxsd::QName> element_stack;  // ancestors
    };

    using ElementCallback =
        std::function<void(const WalkContext&, const ElementVisit&)>;

    class IEPTemplateWalker {
      public:
        explicit IEPTemplateWalker(const nxsd::SchemaRegistry& schema)
            : m_Schema(schema) {}

        void WalkFile(const std::filesystem::path& xmlPath,
                      ElementCallback onElement);

      private:
        const nxsd::SchemaRegistry& m_Schema;

        void WalkNode(tinyxml2::XMLNode* node, WalkContext& ctx,
                      CommentBlock& pendingComments, const ElementCallback& cb);

        nxsd::QName ResolveQName(const tinyxml2::XMLElement* el) const;
    };

}  // namespace iep