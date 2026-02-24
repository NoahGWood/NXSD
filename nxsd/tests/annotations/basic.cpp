#include <NTest.h>
#include <nxsd/nxsd.h>

using namespace nxsd;

TEST(AnnotationStore_Basic_SetGet) {
    AnnotationStore<int> store;

    SchemaNodeRef ref{SCHEMA_NODE_KIND::ELEMENT, {"urn:test", "Foo"}};

    store.Set(ref, 42);

    auto* v = store.Get(ref);
    ASSERT_TRUE(v != nullptr);
    ASSERT_EQ(*v, 42);
}

TEST(AnnotationStore_Overwrite) {
    AnnotationStore<std::string> store;

    SchemaNodeRef ref{SCHEMA_NODE_KIND::TYPE, {"urn:test", "Bar"}};

    store.Set(ref, "first");
    store.Set(ref, "second");

    auto* v = store.Get(ref);
    ASSERT_TRUE(v != nullptr);
    ASSERT_EQ(*v, "second");
}

struct A {
    int x;
};
struct B {
    int y;
};

TEST(AnnotationStore_IsolatedStores) {
    AnnotationStore<A> storeA;
    AnnotationStore<B> storeB;

    SchemaNodeRef ref{SCHEMA_NODE_KIND::ELEMENT, {"urn:test", "Baz"}};

    storeA.Set(ref, A{1});
    storeB.Set(ref, B{2});

    ASSERT_EQ(storeA.Get(ref)->x, 1);
    ASSERT_EQ(storeB.Get(ref)->y, 2);
}

TEST(AnnotationStore_NodeKindDistinction) {
    AnnotationStore<int> store;

    SchemaNodeRef elem{SCHEMA_NODE_KIND::ELEMENT, {"urn:test", "SameName"}};

    SchemaNodeRef type{SCHEMA_NODE_KIND::TYPE, {"urn:test", "SameName"}};

    store.Set(elem, 1);
    store.Set(type, 2);

    ASSERT_EQ(*store.Get(elem), 1);
    ASSERT_EQ(*store.Get(type), 2);
}
