#include <NTest.h>

#include "Helpers.h"

TEST(AnnotationPlugin_NoSchemaMutation) {
    SchemaRegistry schema;
    schema.globalElements[{"urn:test", "Foo"}] = {/* mock */};

    auto before = schema.globalElements.size();

    AnnotationStore<DummyAnnotation> store;
    DummyPlugin plugin;
    plugin.Annotate(schema, store);

    ASSERT_EQ(schema.globalElements.size(), before);
}