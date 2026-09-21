#include <GrevirEncoder.h>
#include <GrevirStepper.h>
#include <GrevirPeripherals.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstdint>

namespace {
struct GPIO {
  inline static std::array<bool, 32> inputs{};
  inline static std::array<bool, 32> levels{};
  static void pinMode(unsigned, ardo::gpio::InputPinMode) {}
  static void pinMode(unsigned, ardo::gpio::OutputPinMode) {}
  static bool digitalRead(unsigned pin) {
    return inputs.at(pin);
  }
  static void digitalWrite(unsigned pin, bool level) {
    levels.at(pin) = level;
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

using QEncoder = quad::QuadEncoder<ardo::InputPin<GPIO, 2>, ardo::InputPin<GPIO, 3>>;
using QModule = quad::QuadEncoderModule<QEncoder>;
using SModule = step::StepperModule<Clock, 20, 50, setl::TimeUnit::MILLIS,
  ardo::OutputPin<GPIO, 5>, ardo::OutputPin<GPIO, 6>,
  ardo::OutputPin<GPIO, 7>, ardo::OutputPin<GPIO, 8>>;

struct CombineModule : ardo::ModuleBase<ardo::Parameters<>> {
  static void runLoop() {
    auto pos = QModule::quadEncoder.getCurrentPosition();
    SModule::instance.stepper.setTargetPosition(pos);
    SModule::instance.stepper.setTimeScale(1.0f);
  }
};

using App = ardo::Application<SModule, QModule, CombineModule>;
} // namespace

TEST_CASE("historical encoder position becomes the stepper target", "[stepper][combine]") {
  Clock::set(0);
  GPIO::inputs.fill(true);
  GPIO::levels.fill(false);
  App::runSetup();
  App::runLoop();
  REQUIRE(SModule::instance.stepper.getCurrentPosition() == 0);
  SModule::instance.stepper.setTargetPosition(2);
  REQUIRE(SModule::instance.stepper.getTargetPosition() == 2);
  GPIO::inputs[2] = false;
  QModule::runLoop();
  REQUIRE(QModule::quadEncoder.getCurrentPosition() == 1);
  CombineModule::runLoop();
  REQUIRE(SModule::instance.stepper.getTargetPosition() == 1);
  Clock::set(21);
  SModule::runLoop();
  REQUIRE(SModule::instance.stepper.getCurrentPosition() == 1);
}
