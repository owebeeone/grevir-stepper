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
using Pin5 = ardo::OutputPin<Backend, 5>;
using Pin6 = ardo::OutputPin<Backend, 6>;
using Pin7 = ardo::OutputPin<Backend, 7>;
using Pin8 = ardo::OutputPin<Backend, 8>;
using Stepper = step::Stepper<Clock, Pin5, Pin6, Pin7, Pin8>;
using Module = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS, Pin5, Pin6, Pin7, Pin8>;
}

static_assert(step::Sequencer<Pin5, Pin6, Pin7, Pin8>::COUNT_PIN == 4);
static_assert(sizeof(step::Sequences<4>::basicSequence()) == 4);
static_assert(sizeof(step::Sequences<2>::basicSequence()) == 4);
static_assert(sizeof(step::Sequences<3>::basicSequence()) == 6);
static_assert(sizeof(step::Sequences<5>::basicSequence()) == 10);

void instantiate_stepper() {
  ardo::Application<Module>::runSetup();
  ardo::Application<Module>::runLoop();
  Module::instance.stepper.setTargetPosition(2);
  Module::instance.stepper.incrementPosition(1);
  Module::instance.stepper.setTimeScale(1.0f);
  Module::instance.stepper.getCurrentPosition();
  Module::instance.stepper.getRemainingSteps();
  Module::instance.stepper.getTargetPosition();
  const unsigned char custom[] = { 0b1000, 0b0100 };
  Stepper custom_stepper{
    typename Stepper::PeriodType(20),
    typename Stepper::PeriodType(0),
    custom};
  custom_stepper.iterate();
}
