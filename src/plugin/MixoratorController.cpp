#include "MixoratorController.h"

namespace Mixorator
{
Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context)
{
    return EditController::initialize(context);
}
}
