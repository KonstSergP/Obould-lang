#include <stdexcept>
#include "TypeInfo.h"


bool TypeInfo::isAssignableFrom(const std::shared_ptr<TypeInfo>& other) const
{
    (void)this; (void)other;
    throw std::runtime_error("not implemented");
    return false;
}
