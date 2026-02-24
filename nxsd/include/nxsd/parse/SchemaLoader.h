#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>

namespace nxsd {

    struct SchemaContext {
        std::string targetNamespace;
        std::unordered_map<std::string, std::string> prefixToNs;
        std::string schemaLocationDir;  // directory of this xsd file
        bool elementFormDefaultQualified = true;
    };

    struct ParseOptions {
        bool follow_includes = true;
        bool follow_imports = true;
    };

    struct SchemaLoader {
        ParseOptions opts;

        bool LoadDirectory(const std::filesystem::path& dir,
                           SchemaRegistry& out);
        bool LoadFile(const std::filesystem::path& xsd, SchemaRegistry& out);
    };

    void ResolveElementRefs(SchemaRegistry& reg);
    void ResolveSimpleContentWrappers(SchemaRegistry& reg);
}  // namespace nxsd
