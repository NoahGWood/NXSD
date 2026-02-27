#include <Helpers.h>
#include <NTest.h>

TEST(ResolveType_PrimitiveAndFacets) {
    SchemaRegistry reg;

    QName T{"test", "T"};

    TypeNode node{};
    node.name = T;
    node.primitive = PrimitiveKind::String;
    node.facets = ValueFacet{};
    node.facets->min_length = 5;
    node.facets->max_length = 10;

    reg.types[T] = node;

    auto rt = ResolveType(T, reg);

    ASSERT_EQ(rt.primitive, PrimitiveKind::String);
    ASSERT_TRUE(rt.facets.has_value());
    ASSERT_EQ(rt.facets->min_length.value(), 5);
    ASSERT_EQ(rt.facets->max_length.value(), 10);
    ASSERT_FALSE(rt.is_complex);
    ASSERT_FALSE(rt.is_union);
}
