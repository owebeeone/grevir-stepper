#include <GrevirStepper.h>
#include <GrevirPeripherals.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace {
struct GPIO {
  inline static std::array<bool, 32> levels{};
  inline static std::vector<std::string> events;

  static void record(unsigned pin, const std::string& action) {
    events.push_back(std::to_string(pin) + ":" + action);
  }
  static void pinMode(unsigned pin, ardo::gpio::OutputPinMode) {
    record(pin, "output");
  }
  static void digitalWrite(unsigned pin, bool level) {
    levels.at(pin) = level;
    record(pin, level ? "high" : "low");
  }
};

struct Clock {
  using TimeType = setl::Time<std::uint32_t>;
  inline static TimeType current{};
  static TimeType now() {
    return current;
  }
  static void set(std::uint32_t ticks) {
    current = TimeType(ticks);
  }
};

using Period = Clock::TimeType::period_type;
using Pin5 = ardo::OutputPin<GPIO, 5>;
using Pin6 = ardo::OutputPin<GPIO, 6>;
using Pin7 = ardo::OutputPin<GPIO, 7>;
using Pin8 = ardo::OutputPin<GPIO, 8>;
using FourPhase = step::Stepper<Clock, Pin5, Pin6, Pin7, Pin8>;
using Module = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS, Pin5, Pin6, Pin7, Pin8>;
using App = ardo::Application<Module>;

struct Fixture {
  Fixture() {
    Clock::set(0);
    GPIO::levels.fill(false);
    GPIO::events.clear();
  }
};

std::array<bool, 4> coil_state() {
  return {GPIO::levels[5], GPIO::levels[6], GPIO::levels[7], GPIO::levels[8]};
}

FourPhase make_stepper(std::uint32_t step, std::uint32_t coil) {
  Clock::set(0);
  return FourPhase{Period(step), Period(coil)};
}
} // namespace

TEST_CASE_METHOD(Fixture, "pin specifier applies the highest bit to the first pin", "[stepper]") {
  step::PinSpecifier<Pin5, Pin6, Pin7, Pin8>::set(0b1100);
  REQUIRE(coil_state() == std::array<bool, 4>{true, true, false, false});
  GPIO::events.clear();
  step::PinSpecifier<Pin5, Pin6, Pin7, Pin8>::setLow();
  REQUIRE(coil_state() == std::array<bool, 4>{false, false, false, false});
}

TEST_CASE_METHOD(Fixture, "no step occurs until elapsed time strictly exceeds the period", "[stepper]") {
  auto stepper = make_stepper(20, 50);
  stepper.setTargetPosition(3);
  stepper.iterate();
  REQUIRE(stepper.getCurrentPosition() == 0);
  REQUIRE(stepper.getRemainingSteps() == 3);
  REQUIRE(GPIO::events.empty());
  Clock::set(20);
  stepper.iterate();
  REQUIRE(stepper.getCurrentPosition() == 0);
  Clock::set(21);
  stepper.iterate();
  REQUIRE(stepper.getCurrentPosition() == 1);
  REQUIRE(stepper.getRemainingSteps() == 2);
  REQUIRE(coil_state() == std::array<bool, 4>{false, true, true, false});
}

TEST_CASE_METHOD(Fixture, "forward steps wrap the four-phase sequence", "[stepper]") {
  auto stepper = make_stepper(20, 0);
  stepper.setTargetPosition(4);
  const std::array<std::array<bool, 4>, 4> rows{{
    {false, true, true, false},
    {false, false, true, true},
    {true, false, false, true},
    {true, true, false, false},
  }};
  for (unsigned step = 0; step < 4; ++step) {
    Clock::set(21u + step * 21u);
    stepper.iterate();
    REQUIRE(stepper.getCurrentPosition() == static_cast<long>(step + 1));
    REQUIRE(coil_state() == rows[step]);
  }
  REQUIRE(stepper.getRemainingSteps() == 0);
}

TEST_CASE_METHOD(Fixture, "negative remaining steps walk the sequence backward", "[stepper]") {
  auto stepper = make_stepper(20, 0);
  stepper.setTargetPosition(-1);
  Clock::set(21);
  stepper.iterate();
  REQUIRE(stepper.getCurrentPosition() == -1);
  REQUIRE(coil_state() == std::array<bool, 4>{true, false, false, true});
}

TEST_CASE_METHOD(Fixture, "coils turn off after the hold period and stay on when hold is zero", "[stepper]") {
  auto timed = make_stepper(20, 50);
  timed.setTargetPosition(1);
  Clock::set(21);
  timed.iterate();
  REQUIRE(coil_state() == std::array<bool, 4>{false, true, true, false});
  Clock::set(71);
  timed.iterate();
  REQUIRE(coil_state() == std::array<bool, 4>{false, true, true, false});
  Clock::set(72);
  timed.iterate();
  REQUIRE(coil_state() == std::array<bool, 4>{false, false, false, false});

  auto held = make_stepper(20, 0);
  held.setTargetPosition(1);
  Clock::set(21);
  held.iterate();
  Clock::set(1000);
  held.iterate();
  REQUIRE(coil_state() == std::array<bool, 4>{false, true, true, false});
}

TEST_CASE_METHOD(Fixture, "time scale lengthens the wait between steps", "[stepper]") {
  auto stepper = make_stepper(20, 0);
  stepper.setTimeScale(2.0f);
  REQUIRE(stepper.getTimeScale() == 2.0f);
  stepper.setTargetPosition(1);
  Clock::set(21);
  stepper.iterate();
  REQUIRE(stepper.getCurrentPosition() == 0);
  Clock::set(41);
  stepper.iterate();
  REQUIRE(stepper.getCurrentPosition() == 1);
}

TEST_CASE_METHOD(Fixture, "target increment and remaining steps stay consistent", "[stepper]") {
  auto stepper = make_stepper(20, 0);
  stepper.setTargetPosition(2);
  REQUIRE(stepper.getTargetPosition() == 2);
  stepper.incrementPosition(3);
  REQUIRE(stepper.getRemainingSteps() == 5);
  REQUIRE(stepper.getTargetPosition() == 5);
  Clock::set(21);
  stepper.iterate();
  REQUIRE(stepper.getCurrentPosition() == 1);
  REQUIRE(stepper.getRemainingSteps() == 4);
  REQUIRE(stepper.getTargetPosition() == 5);
}

TEST_CASE_METHOD(Fixture, "idle state machine reports no scheduled action", "[stepper]") {
  step::StepperStateMachine<Clock> machine{Period(20), Period(50)};
  REQUIRE(machine.nextActionPeriodUnits() == Period(~Period::type(0)));
  machine.setTargetPosition(1);
  REQUIRE(machine.nextActionPeriodUnits().get() == 20);
  Clock::set(21);
  REQUIRE(machine.nextActionPeriodUnits() == Period(0));
}

TEST_CASE_METHOD(Fixture, "custom sequence uses caller rows and two-phase tables exist", "[stepper]") {
  static const unsigned char custom[] = { 0b1000, 0b0001 };
  FourPhase stepper{Period(10), Period(0), custom};
  stepper.setTargetPosition(1);
  Clock::set(11);
  stepper.iterate();
  REQUIRE(coil_state() == std::array<bool, 4>{false, false, false, true});
  REQUIRE(step::lengthOf(step::Sequences<2>::basicSequence()) == 4);
}

TEST_CASE_METHOD(Fixture, "module setup claims outputs and loop follows the historical harness", "[stepper]") {
  App::runSetup();
  REQUIRE(GPIO::events == std::vector<std::string>{
    "8:output", "7:output", "6:output", "5:output"});
  Module::instance.stepper.setTargetPosition(2);
  App::runLoop();
  REQUIRE(Module::instance.stepper.getCurrentPosition() == 0);
  Clock::set(21);
  App::runLoop();
  REQUIRE(Module::instance.stepper.getCurrentPosition() == 1);
  Clock::set(42);
  App::runLoop();
  REQUIRE(Module::instance.stepper.getCurrentPosition() == 2);
}

TEST_CASE_METHOD(Fixture, "two stepper modules keep independent positions", "[stepper]") {
  using First = step::StepperModule<Clock, 20, 0, setl::TimeUnit::MILLIS,
    ardo::OutputPin<GPIO, 1>, ardo::OutputPin<GPIO, 2>>;
  using Second = step::StepperModule<Clock, 20, 0, setl::TimeUnit::MILLIS,
    ardo::OutputPin<GPIO, 3>, ardo::OutputPin<GPIO, 4>>;
  using Dual = ardo::Application<First, Second>;
  Dual::runSetup();
  First::instance.stepper.setTargetPosition(1);
  Second::instance.stepper.setTargetPosition(-1);
  Clock::set(21);
  Dual::runLoop();
  REQUIRE(First::instance.stepper.getCurrentPosition() == 1);
  REQUIRE(Second::instance.stepper.getCurrentPosition() == -1);
  REQUIRE(GPIO::levels[1]);
  REQUIRE(GPIO::levels[2]);
  REQUIRE_FALSE(GPIO::levels[3]);
  REQUIRE_FALSE(GPIO::levels[4]);
}
