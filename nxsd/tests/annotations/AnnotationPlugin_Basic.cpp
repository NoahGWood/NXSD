#include <NTest.h>

#include "Helpers.h"

TEST(AnnotationPlugin_Basic) {
    SchemaRegistry schema;
    schema.globalElements[{"urn:test", "Foo"}] = {/* mock */};

    AnnotationStore<DummyAnnotation> store;
    DummyPlugin plugin;

    plugin.Annotate(schema, store);

    SchemaNodeRef ref{SCHEMA_NODE_KIND::ELEMENT, {"urn:test", "Foo"}};

    ASSERT_TRUE(store.Get(ref) != nullptr);
    ASSERT_EQ(store.Get(ref)->value, 123);
}