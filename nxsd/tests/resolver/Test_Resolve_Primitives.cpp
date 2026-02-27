#include <Helpers.h>
#include <NTest.h>

TEST(XSD_ResolvePrimitive_DirectPrimitive) {
    TypeRegistry reg;

    TypeNode stringType;
    stringType.name = Q("xs", "string");
    stringType.primitive = PrimitiveKind::String;

    reg[stringType.name] = stringType;

    auto pk = ResolvePrimitive(Q("xs", "string"), reg);
    ASSERT_EQ(pk, PrimitiveKind::String);
}

TEST(XSD_ResolvePrimitive_UnknownType) {
    TypeRegistry reg;

    auto pk = ResolvePrimitive(Q("ex", "DoesNotExist"), reg);
    ASSERT_EQ(pk, PrimitiveKind::Unknown);
}

TEST(XSD_ResolvePrimitive_ComplexType) {
    TypeRegistry reg;

    TypeNode complex;
    complex.name = Q("ex", "PersonType");
    complex.elements.push_back({.name = Q("ex", "Name"),
                                .type = Q("xs", "string"),
                                .min_occurs = 1,
                                .max_occurs = 1});

    reg[complex.name] = complex;

    auto pk = ResolvePrimitive(complex.name, reg);
    ASSERT_EQ(pk, PrimitiveKind::Unknown);
}

TEST(XSD_ResolvePrimitive_CycleSafe) {
    TypeRegistry reg;

    TypeNode a;
    a.name = Q("ex", "A");
    a.base_types.push_back(Q("ex", "B"));

    TypeNode b;
    b.name = Q("ex", "B");
    b.base_types.push_back(Q("ex", "A"));

    reg[a.name] = a;
    reg[b.name] = b;

    auto pk = ResolvePrimitive(a.name, reg);
    ASSERT_EQ(pk, PrimitiveKind::Unknown);
}

TEST(XSD_ResolvePrimitive_UnionType) {
    TypeRegistry reg;

    TypeNode intType;
    intType.name = Q("xs", "integer");
    intType.primitive = PrimitiveKind::Integer;

    TypeNode strType;
    strType.name = Q("xs", "string");
    strType.primitive = PrimitiveKind::String;

    TypeNode unionType;
    unionType.name = Q("ex", "IntOrString");
    unionType.union_types.push_back(intType.name);
    unionType.union_types.push_back(strType.name);

    reg[intType.name] = intType;
    reg[strType.name] = strType;
    reg[unionType.name] = unionType;

    auto pk = ResolvePrimitive(unionType.name, reg);
    ASSERT_NE(pk, PrimitiveKind::Unknown);
}

TEST(ResolvePrimitive_InheritsThroughChain) {
    SchemaRegistry reg;

    QName A{"test", "A"};
    QName B{"test", "B"};
    QName C{"test", "C"};

    TypeNode typeA{};
    typeA.name = A;
    typeA.primitive = PrimitiveKind::Integer;

    TypeNode typeB{};
    typeB.name = B;
    typeB.base_types.push_back(A);

    TypeNode typeC{};
    typeC.name = C;
    typeC.base_types.push_back(B);

    reg.types[A] = typeA;
    reg.types[B] = typeB;
    reg.types[C] = typeC;

    auto p = ResolvePrimitive(C, reg.types);

    ASSERT_EQ(p, PrimitiveKind::Integer);
}