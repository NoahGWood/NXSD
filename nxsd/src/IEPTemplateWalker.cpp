#include <nxsd/parse/IEPTemplateWalker.h>

namespace iep {

    void iep::IEPTemplateWalker::WalkFile(const std::filesystem::path& xmlPath,
                                          ElementCallback onElement) {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(xmlPath.string().c_str()) != tinyxml2::XML_SUCCESS)
            return;

        WalkContext ctx;
        CommentBlock comments;

        WalkNode(doc.RootElement(), ctx, comments, onElement);
    }

    void IEPTemplateWalker::WalkNode(tinyxml2::XMLNode* node, WalkContext& ctx,
                                     CommentBlock& pendingComments,
                                     const ElementCallback& cb) {
        if (!node)
            return;

        if (auto* comment = node->ToComment()) {
            std::string txt = comment->Value();
            if (!txt.empty()) {
                pendingComments.lines.push_back(txt);
                return;
            }
            auto* el = node->ToElement();
            if (el) {
                nxsd::QName qn = ResolveQName(el);

                ctx.element_stack.push_back(qn);

                ElementVisit visit{
                    .name = qn, .comments = pendingComments, .xml = el};

                cb(ctx, visit);

                pendingComments.lines.clear();
            }

            for (auto* c = node->FirstChild(); c; c = c->NextSibling())
                WalkNode(c, ctx, pendingComments, cb);

            if (el)
                ctx.element_stack.pop_back();
        }
    }
    nxsd::QName IEPTemplateWalker::ResolveQName(
        const tinyxml2::XMLElement* el) const {
        nxsd::QName q;

        const char* raw = el->Name();
        if (!raw)
            return q;

        std::string full(raw);

        auto pos = full.find(':');
        if (pos == std::string::npos) {
            q.local = full;
            return q;
        }

        std::string prefix = full.substr(0, pos);
        q.local = full.substr(pos + 1);

        // Walk up DOM to find xmlns:prefix declaration
        const tinyxml2::XMLNode* cur = el;
        while (cur) {
            const tinyxml2::XMLElement* e = cur->ToElement();
            if (e) {
                std::string attrName = "xmlns:" + prefix;
                const char* ns = e->Attribute(attrName.c_str());
                if (ns) {
                    q.ns = ns;
                    return q;
                }
            }
            cur = cur->Parent();
        }

        return q;  // ns left empty if not found
    }

}  // namespace iep
