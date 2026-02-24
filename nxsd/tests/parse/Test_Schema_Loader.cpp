#include <Helpers.h>
#include <NTest.h>
#include <tinyxml2.h>

TEST(NXSD_Load_IncludeCycleSafe) {
    auto a = WriteTempXsd("a.xsd", R"(
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
          <xs:include schemaLocation="b.xsd"/>
        </xs:schema>
    )");

    auto b = WriteTempXsd("b.xsd", R"(
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
          <xs:include schemaLocation="a.xsd"/>
        </xs:schema>
    )");

    SchemaLoader loader;
    SchemaRegistry reg;
    ASSERT_TRUE(loader.LoadFile(a, reg));
}

TEST(NXSD_Load_SimpleTypeRestriction) {
    auto xsd = WriteTempXsd("simple_restriction.xsd", R"(
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"
                   targetNamespace="urn:test"
                   xmlns="urn:test">

          <xs:simpleType name="AgeType">
            <xs:restriction base="xs:nonNegativeInteger">
              <xs:maxInclusive value="99"/>
            </xs:restriction>
          </xs:simpleType>

        </xs:schema>
    )");

    SchemaLoader loader;
    SchemaRegistry reg;

    ASSERT_TRUE(loader.LoadFile(xsd, reg));

    QName q{"urn:test", "AgeType"};
    ASSERT_TRUE(reg.types.contains(q));

    const TypeNode& t = reg.types.at(q);

    ASSERT_TRUE(t.IsSimple());
    ASSERT_TRUE(t.primitive.has_value());
    ASSERT_EQ(*t.primitive, PrimitiveKind::Integer);

    ASSERT_TRUE(t.facets.has_value());
    ASSERT_TRUE(t.facets->max_inclusive.has_value());
    ASSERT_EQ(*t.facets->max_inclusive, 99);
}

TEST(NXSD_Load_Enumeration) {
    auto xsd = WriteTempXsd("enum.xsd", R"(
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"
                   targetNamespace="urn:test"
                   xmlns="urn:test">

          <xs:simpleType name="EyeColor">
            <xs:restriction base="xs:string">
              <xs:enumeration value="Blue"/>
              <xs:enumeration value="Green"/>
              <xs:enumeration value="Brown"/>
            </xs:restriction>
          </xs:simpleType>

        </xs:schema>
    )");

    SchemaLoader loader;
    SchemaRegistry reg;
    ASSERT_TRUE(loader.LoadFile(xsd, reg));

    QName q{"urn:test", "EyeColor"};
    const TypeNode& t = reg.types.at(q);

    ASSERT_TRUE(t.enums.has_value());
    ASSERT_EQ(t.enums->options.size(), 3);
    ASSERT_EQ(t.enums->options[0], "Blue");
}

TEST(NXSD_Load_UnionType) {
    auto xsd = WriteTempXsd("union.xsd", R"(
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"
                   targetNamespace="urn:test"
                   xmlns="urn:test">

          <xs:simpleType name="IntOrString">
            <xs:union memberTypes="xs:int xs:string"/>
          </xs:simpleType>

        </xs:schema>
    )");

    SchemaLoader loader;
    SchemaRegistry reg;
    ASSERT_TRUE(loader.LoadFile(xsd, reg));

    QName q{"urn:test", "IntOrString"};
    const TypeNode& t = reg.types.at(q);

    ASSERT_EQ(t.union_types.size(), 2);
    ASSERT_EQ(t.union_types[0].local, "int");
    ASSERT_EQ(t.union_types[1].local, "string");
}

TEST(NXSD_Load_ComplexSequence) {
    auto xsd = WriteTempXsd("complex_seq.xsd", R"(
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"
                   targetNamespace="urn:test"
                   xmlns="urn:test">

          <xs:complexType name="Person">
            <xs:sequence>
              <xs:element name="Name" type="xs:string"/>
              <xs:element name="Age" type="xs:int" minOccurs="0"/>
              <xs:element name="Alias" type="xs:string" maxOccurs="unbounded"/>
            </xs:sequence>
          </xs:complexType>

        </xs:schema>
    )");

    SchemaLoader loader;
    SchemaRegistry reg;
    ASSERT_TRUE(loader.LoadFile(xsd, reg));

    QName q{"urn:test", "Person"};
    const TypeNode& t = reg.types.at(q);

    ASSERT_TRUE(t.IsComplex());
    ASSERT_EQ(t.elements.size(), 3);

    const ElementNode& age = t.elements[1];
    ASSERT_EQ(age.min_occurs, 0);
    ASSERT_EQ(age.max_occurs, 1);

    const ElementNode& alias = t.elements[2];
    ASSERT_EQ(alias.max_occurs, -1);  // unbounded
}

TEST(NXSD_Load_Extension) {
    auto xsd = WriteTempXsd("extension.xsd", R"(
        <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"
                   targetNamespace="urn:test"
                   xmlns="urn:test">

          <xs:complexType name="Base">
            <xs:sequence>
              <xs:element name="ID" type="xs:int"/>
            </xs:sequence>
          </xs:complexType>

          <xs:complexType name="Derived">
            <xs:complexContent>
              <xs:extension base="Base">
                <xs:sequence>
                  <xs:element name="Extra" type="xs:string"/>
                </xs:sequence>
              </xs:extension>
            </xs:complexContent>
          </xs:complexType>

        </xs:schema>
    )");

    SchemaLoader loader;
    SchemaRegistry reg;
    ASSERT_TRUE(loader.LoadFile(xsd, reg));

    QName q{"urn:test", "Derived"};
    const TypeNode& t = reg.types.at(q);

    ASSERT_EQ(t.base_types.size(), 1);
    ASSERT_EQ(t.base_types[0].local, "Base");
    ASSERT_EQ(t.elements.size(), 1);
}

TEST(NXSD_Load_FileFromTestDir) {
    std::filesystem::path testDir = NXSD_TEST_DATA_DIR;
    std::filesystem::path xsdPath = testDir / "basic/basic_types.xsd";
    // EBTS 11.2 XML IEPD_20250212

    ASSERT_TRUE(std::filesystem::exists(xsdPath));

    SchemaLoader loader;
    SchemaRegistry reg;

    bool ok = loader.LoadFile(xsdPath, reg);
    ASSERT_TRUE(ok);

    // Verify something meaningful was loaded
    QName q{"urn:test", "TestInt"};
    ASSERT_TRUE(reg.types.contains(q));

    const TypeNode& t = reg.types.at(q);
    ASSERT_TRUE(t.primitive.has_value());
    ASSERT_EQ(*t.primitive, PrimitiveKind::Integer);
}

// TEST(NXSD_Load_EBTSFilesFromTestDir) {
//     std::filesystem::path testDir = NXSD_TEST_DATA_DIR;
//     // std::filesystem::path xsdPath =
//     // testDir / "EBTS 11.2 XML IEPD_20250212/mpd-catalog.xml";
//     std::filesystem::path xsdPath =
//         testDir /
//         "EBTS 11.2 XML IEPD_20250212/base-xsd/fbi_ebts/11.2/fbi_ebts.xsd";
//     ASSERT_TRUE(std::filesystem::exists(xsdPath));

//     SchemaLoader loader;
//     SchemaRegistry reg;

//     bool ok = loader.LoadFile(xsdPath, reg);
//     ASSERT_TRUE(ok);

//     DumpRegistry(reg);  // <—— stdout dump
// }

TEST(NXSD_Load_EBTS_FromMPD_DiscoveredSchemas) {
    std::filesystem::path testDir = NXSD_TEST_DATA_DIR;
    std::filesystem::path mpdPath =
        testDir / "EBTS 11.2 XML IEPD_20250212/mpd-catalog.xml";

    ASSERT_TRUE(std::filesystem::exists(mpdPath));

    // ---- Step 1: Discover schema paths from MPD ----
    std::vector<std::filesystem::path> schemas;

    {
        tinyxml2::XMLDocument doc;
        ASSERT_EQ(doc.LoadFile(mpdPath.string().c_str()),
                  tinyxml2::XML_SUCCESS);

        auto* root = doc.RootElement();
        ASSERT_TRUE(root);

        for (auto* el = root->FirstChildElement(); el;
             el = el->NextSiblingElement()) {
            // Walk descendants looking for c:XMLSchemaDocument /
            // c:ExtensionSchemaDocument
            std::function<void(tinyxml2::XMLElement*)> walk =
                [&](tinyxml2::XMLElement* n) {
                    if (!n)
                        return;

                    std::string local = n->Name();
                    auto pos = local.find(':');
                    if (pos != std::string::npos)
                        local = local.substr(pos + 1);

                    if (local == "XMLSchemaDocument" ||
                        local == "ExtensionSchemaDocument") {
                        const char* path = n->Attribute("c:pathURI");
                        if (path) {
                            schemas.push_back(mpdPath.parent_path() / path);
                        }
                    }

                    for (auto* c = n->FirstChildElement(); c;
                         c = c->NextSiblingElement())
                        walk(c);
                };

            walk(el);
        }
    }

    ASSERT_FALSE(schemas.empty());

    // ---- Step 2: Load discovered schemas via NXSD ----
    SchemaLoader loader;
    SchemaRegistry reg;

    for (const auto& xsd : schemas) {
        ASSERT_TRUE(std::filesystem::exists(xsd));
        ASSERT_TRUE(loader.LoadFile(xsd, reg));
    }

    // std::cout << "EBTS types loaded: " << reg.types.size() << "\n";
    DumpRegistry(reg.types);

    ASSERT_FALSE(reg.types.empty());
}
