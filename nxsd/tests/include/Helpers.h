#include <nxsd/nxsd.h>

#include <filesystem>
#include <fstream>
#include <string>
using namespace nxsd;

static QName Q(std::string ns, std::string local) {
    return QName{ns, local};
}

static std::filesystem::path WriteTempXsd(
    const std::string& name,
    const std::string& contents)
{
    auto dir = std::filesystem::temp_directory_path() / "nxsd_tests";
    std::filesystem::create_directories(dir);

    auto path = dir / name;
    std::ofstream f(path);
    f << contents;
    f.close();

    return path;
}


struct DummyAnnotation {
    int value;
};

struct DummyPlugin : IAnnotation<DummyAnnotation> {
    void Annotate(const SchemaRegistry& schema,
                  AnnotationStore<DummyAnnotation>& store) override {
        for (auto& [qname, el] : schema.globalElements) {
            store.Set(
                {SCHEMA_NODE_KIND::ELEMENT, qname},
                DummyAnnotation{123}
            );
        }
    }
};