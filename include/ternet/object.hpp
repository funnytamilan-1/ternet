#pragma once
#include "value.hpp"

namespace ternet {
// Stable V1 object façade. Heap-backed object specialization will build on this API.
using Object = Value::Object;
}
