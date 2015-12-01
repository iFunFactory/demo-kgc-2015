// PLEASE ADD YOUR EVENT HANDLER DECLARATIONS HERE.

#include <boost/bind.hpp>
#include <funapi.h>
#include <gflags/gflags.h>

#include "event_handlers.h"
#include "unitychan_object.h"


// You can differentiate game server flavors.
DECLARE_string(app_flavor);

// Adding gflags. In your code, you can refer to them as FLAGS_example_arg3, ...
DEFINE_string(example_arg3, "default_val", "example flag");
DEFINE_int32(example_arg4, 100, "example flag");
DEFINE_bool(example_arg5, false, "example flag");


namespace {

const WallClock::Duration kOneSecond = WallClock::FromMsec(1000);

class UnitychanServer : public Component {
 public:
  static bool Install(const ArgumentMap &arguments) {
    LOG(INFO) << "Built using Engine version: " << FUNAPI_BUILD_IDENTIFIER;

    // Kickstarts the Engine's ORM.
    // Do not touch this, unless you fully understand what you are doing.
    unitychan::ObjectModelInit();

    /*
     * Parameters specified in the "arguments" section in your MANIFEST.json
     * will be passed in the variable "arguments".
     * So, you can give configuration data to your game server.
     *
     * Example:
     *
     * We have in MANIFEST.json "example_arg1" and "example_arg2" that
     * have a string value and an integer value, respectively.
     * So, you can access the arguments like below:
     */
    string arg1 = arguments.FindStringArgument("example_arg1");
    LOG(INFO) << "example_arg1: " << arg1;

    int64_t arg2 = arguments.FindIntegerArgument("example_arg2");
    LOG(INFO) << "example_arg2: " << arg2;

     // You can override gflag like this: ./unitychan-local --example_arg3=hahaha
    LOG(INFO) << "example_arg3: " << FLAGS_example_arg3;


    /*
     * Registers various handlers.
     * You may be interesed in this function and handlers in it.
     * Please see "event_handlers.cc"
     */
    unitychan::RegisterEventHandlers();

    return true;
  }

  static bool Start() {
    return true;
  }

  static bool Uninstall() {
    return true;
  }
};

}  // unnamed namespace


REGISTER_STARTABLE_COMPONENT(UnitychanServer, UnitychanServer)
