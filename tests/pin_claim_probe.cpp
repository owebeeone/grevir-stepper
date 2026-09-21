#include <GrevirStepper.h>
#include <GrevirPeripherals.h>
#include <cstdint>

namespace {
struct Backend {
  static void pinMode(unsigned, ardo::gpio::OutputPinMode);
  static void digitalWrite(unsigned, bool);
};
struct Clock {
  using TimeType = setl::Time<std::uint32_t>;
  static TimeType now();
};

#if CASE_ID == 0
using Module = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS,
  ardo::OutputPin<Backend, 5>, ardo::OutputPin<Backend, 6>,
  ardo::OutputPin<Backend, 7>, ardo::OutputPin<Backend, 8>>;
using App = ardo::Application<Module>;
#elif CASE_ID == 1
using First = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS,
  ardo::OutputPin<Backend, 1>, ardo::OutputPin<Backend, 2>>;
using Second = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS,
  ardo::OutputPin<Backend, 3>, ardo::OutputPin<Backend, 4>>;
using App = ardo::Application<First, Second>;
#elif CASE_ID == 2
using Module = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS,
  ardo::OutputPin<Backend, 5>, ardo::OutputPin<Backend, 5>>;
using App = ardo::Application<Module>;
#elif CASE_ID == 3
using First = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS,
  ardo::OutputPin<Backend, 5>, ardo::OutputPin<Backend, 6>>;
using Second = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS,
  ardo::OutputPin<Backend, 6>, ardo::OutputPin<Backend, 7>>;
using App = ardo::Application<First, Second>;
#endif

void instantiate_lifecycle() {
  App::runSetup();
  App::runLoop();
}
}
