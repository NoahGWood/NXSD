#include <nxsd/data/Registry.h>

#include <unordered_set>
namespace nxsd {

    void DumpRegistry(const TypeRegistry& reg);
    void DumpTypeTree(const QName& q, const TypeRegistry& reg,
                      std::unordered_set<QName, QNameHash>& seen,
                      int indent = 0);

}  // namespace nxsd
