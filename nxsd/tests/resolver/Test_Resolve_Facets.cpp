#include <Helpers.h>
#include <NTest.h>

TEST(XSD_ResolveFacets_DirectFacet) {
    TypeRegistry reg;

    TypeNode t;
    t.name = Q("ex", "AgeType");
    t.facets = ValueFacet{.min_inclusive = 0, .max_inclusive = 120};

    reg[t.name] = t;

    auto f = ResolveFacets(t.name, reg);
    ASSERT_TRUE(f.has_value());
    ASSERT_EQ(f->min_inclusive.value(), 0);
    ASSERT_EQ(f->max_inclusive.value(), 120);
}

TEST(XSD_ResolveFacets_InheritedFacet) {
    TypeRegistry reg;

    TypeNode base;
    base.name = Q("xs", "positiveInteger");
    base.facets = ValueFacet{.min_inclusive = 1};

    TypeNode derived;
    derived.name = Q("ex", "Integer1to99Type");
    derived.base_types.push_back(base.name);
    derived.facets = ValueFacet{.max_inclusive = 99};

    reg[base.name] = base;
    reg[derived.name] = derived;

    auto f = ResolveFacets(derived.name, reg);
    ASSERT_TRUE(f.has_value());
    ASSERT_EQ(f->min_inclusive.value(), 1);
    ASSERT_EQ(f->max_inclusive.value(), 99);
}

TEST(XSD_ResolveFacets_MultiLevelInheritance) {
    TypeRegistry reg;

    TypeNode integer;
    integer.name = Q("xs", "integer");

    TypeNode nonNeg;
    nonNeg.name = Q("xs", "nonNegativeInteger");
    nonNeg.base_types.push_back(integer.name);
    nonNeg.facets = ValueFacet{.min_inclusive = 0};

    TypeNode small;
    small.name = Q("ex", "SmallInt");
    small.base_types.push_back(nonNeg.name);
    small.facets = ValueFacet{.max_inclusive = 10};

    reg[integer.name] = integer;
    reg[nonNeg.name] = nonNeg;
    reg[small.name] = small;

    auto f = ResolveFacets(small.name, reg);
    ASSERT_TRUE(f.has_value());
    ASSERT_EQ(f->min_inclusive.value(), 0);
    ASSERT_EQ(f->max_inclusive.value(), 10);
}

TEST(XSD_ResolveFacets_NoneFound) {
    TypeRegistry reg;

    TypeNode t;
    t.name = Q("ex", "FreeFormText");

    reg[t.name] = t;

    auto f = ResolveFacets(t.name, reg);
    ASSERT_FALSE(f.has_value());
}

TEST(FacetMerge_TightensNumericBounds) {
    ValueFacet base{};
    base.min_inclusive = 0;
    base.max_inclusive = 100;

    ValueFacet derived{};
    derived.min_inclusive = 10;
    derived.max_inclusive = 90;

    MergeFacet(base, derived);

    ASSERT_EQ(base.min_inclusive.value(), 10);
    ASSERT_EQ(base.max_inclusive.value(), 90);
}

TEST(FacetMerge_TightensLengthBounds) {
    ValueFacet base{};
    base.min_length = 1;
    base.max_length = 20;

    ValueFacet derived{};
    derived.min_length = 5;
    derived.max_length = 10;

    MergeFacet(base, derived);

    ASSERT_EQ(base.min_length.value(), 5);
    ASSERT_EQ(base.max_length.value(), 10);
}

TEST(FacetMerge_DoesNotWiden) {
    ValueFacet base{};
    base.min_inclusive = 10;
    base.max_inclusive = 50;

    ValueFacet derived{};
    derived.min_inclusive = 0;   // attempt to widen
    derived.max_inclusive = 100; // attempt to widen

    MergeFacet(base, derived);

    // Should remain tightened
    ASSERT_EQ(base.min_inclusive.value(), 10);
    ASSERT_EQ(base.max_inclusive.value(), 50);
}

TEST(ResolveFacets_MergesAcrossChain) {
    SchemaRegistry reg;

    QName A{"test", "A"};
    QName B{"test", "B"};

    TypeNode base{};
    base.name = A;
    base.primitive = PrimitiveKind::Integer;
    base.facets = ValueFacet{};
    base.facets->min_inclusive = 0;
    base.facets->max_inclusive = 100;

    TypeNode derived{};
    derived.name = B;
    derived.base_types.push_back(A);
    derived.facets = ValueFacet{};
    derived.facets->min_inclusive = 10;

    reg.types[A] = base;
    reg.types[B] = derived;

    auto facets = ResolveFacets(B, reg.types);

    ASSERT_TRUE(facets.has_value());
    ASSERT_EQ(facets->min_inclusive.value(), 10);
    ASSERT_EQ(facets->max_inclusive.value(), 100);
}