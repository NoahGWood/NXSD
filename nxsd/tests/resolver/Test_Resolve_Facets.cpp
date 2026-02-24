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
