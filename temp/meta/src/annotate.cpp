#include "meta/annotate.hpp"

using namespace sm;
using namespace sm::reflect;

const void *Annotated::findAnnotation(const std::type_info& type) const noexcept {
    for (const Annotation& annotation : mAnnotations) {
        if (annotation.type == type) {
            return annotation.value;
        }
    }

    return nullptr;
}
