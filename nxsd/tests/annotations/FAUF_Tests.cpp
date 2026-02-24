#include <NTest.h>

#include "FAUFPlugin.h"

TEST(CompileTransaction_FAUF_Minimal) {
    SchemaRegistry schema = LoadTestSchema();
    AnnotationStore<EbtsFieldAnnotation> ebts;
    LoadEbtsAnnotations(schema, ebts);  // real plugin if possible

    TransactionProfile p = CompileTransactionProfile(schema, ebts, "FAUF");

    ASSERT_EQ(p.transaction_code, "FAUF");
    ASSERT_TRUE(p.records.contains(1));
    ASSERT_TRUE(p.records.contains(2));
    ASSERT_TRUE(p.records.contains(14));
}

TEST(CompileTransaction_FAUF_RecordCardinality) {
    SchemaRegistry schema = LoadTestSchema();
    AnnotationStore<EbtsFieldAnnotation> ebts;
    LoadEbtsAnnotations(schema, ebts);  // real plugin if possible

    TransactionProfile p = CompileTransactionProfile(schema, ebts, "FAUF");

    ASSERT_EQ(p.records.at(1).min_occurs, 1);
    ASSERT_EQ(p.records.at(1).max_occurs, 1);

    ASSERT_EQ(p.records.at(14).min_occurs, 1);
    ASSERT_EQ(p.records.at(14).max_occurs, 10);
}

TEST(CompileTransaction_FAUF_FieldPresence) {
    SchemaRegistry schema = LoadTestSchema();
    AnnotationStore<EbtsFieldAnnotation> ebts;
    // LoadEbtsAnnotations(schema, ebts);  // real plugin if possible
    LoadEbtsAnnotationsFromTemplate(
        schema, ebts,
        "nxsd/tests/data/EBTS 11.2 XML IEPD_20250212/iep-sample/Identification "
        "Service/Template(FAUF)FederalApplicantUserFeeTransaction.xml");

    std::cerr << "\n=== EBTS ANNOTATIONS ===\n";
    ebts.ForEach([](const SchemaNodeRef& ref, const EbtsFieldAnnotation& ann) {
        std::cerr << "NODE: "
                  << (ref.kind == SCHEMA_NODE_KIND::ELEMENT ? "ELEMENT"
                                                            : "TYPE")
                  << "  " << ref.name.ns << ":" << ref.name.local
                  << "  EBTS=" << ann.ebts_id << "  REC=" << ann.record_type
                  << "\n";
    });
    std::cerr << "=======================\n";
    TransactionProfile p = CompileTransactionProfile(schema, ebts, "FAUF");

    auto& type2 = p.records.at(2);
    ASSERT_TRUE(type2.fields.contains("2.016"));
    std::cout << type2.fields.at("2.016").element.ns
              << type2.fields.at("2.016").element.local << "\n";
    const auto& f = type2.fields.at("2.016");
    ASSERT_EQ(f.record_type, 2);
}