#include "scripting/Script.h"

namespace scripting {

Script::Script(const std::string& name, ScriptType type)
    : name_(name)
    , type_(type)
    , enabled_(true) {
}

Script::~Script() {
}

} // namespace scripting
