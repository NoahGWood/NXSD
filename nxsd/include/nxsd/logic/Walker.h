
#pragma once
#include <functional>

#include "../data/Registry.h"

namespace nxsd {
    namespace nxsd {

        void WalkType(const QName& type, const TypeRegistry& reg,
                      const std::function<void(const TypeNode&)>& visitor);

    }

}  // namespace nxsd
