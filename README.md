# Grevir Stepper

Stepper phase tables, movement state and a Core module wrapper, extracted from
Ardoinus `ardOStepper.h`. Public types retain the `step` names. Pins and clocks
are injected; the package has no Arduino, FastLED or MCU-backend dependency.
Encoder is not a production dependency. The combined encoder-follower sketch
stays planned until Arduino examples are in scope.

`GrevirStepper.h` includes `<grevir/stepper/stepper.hpp>`. Production use needs
Grevir Base, Time, Core and Peripherals. Callers supply output pin types with
static `set(bool)` and Core GPIO claims, typically `ardo::OutputPin<Backend, N>`.

## Installed use

```cmake
find_package(grevir-stepper CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE grevir::stepper)
```

The target supplies C++23 and its Grevir dependencies. Normal production builds
and installed consumers do not require Catch2, Test Support or Encoder. Arduino
library layout is supplied; Arduino and target builds are not validated.

```cpp
#include <GrevirStepper.h>
#include <GrevirPeripherals.h>

using Module = step::StepperModule<BoardClock, 20, 50, setl::TimeUnit::MILLIS,
  ardo::OutputPin<BoardGPIO, 5>,
  ardo::OutputPin<BoardGPIO, 6>,
  ardo::OutputPin<BoardGPIO, 7>,
  ardo::OutputPin<BoardGPIO, 8>>;
using App = ardo::Application<Module>;
```

See [the installed consumer](tests/installed-consumer/main.cpp) for a host
application that takes two steps. A clock supplies `TimeType` and `now()`.

## Movement contract

- The first pin is the highest bit of each sequence mask. Default tables exist
  for 2, 3, 4 and 5 phases. A custom row array may be passed to `Stepper`.
- `iterate()` is polled. A step is taken when elapsed time is **strictly greater**
  than the scaled step period. The first forward step applies sequence row 1, not
  row 0; row 0 is used after wrap. Reverse from row 0 wraps to the last row.
- `setTargetPosition` / `incrementPosition` queue remaining steps. Position counts
  steps taken, not electrical degrees.
- `setTimeScale` is an explicit float multiplier of the step period (inverse of
  speed). Values below 1 are not rejected. The product is converted back to the
  clock's integer period type.
- A positive coil-hold period turns coils off after that idle time using strict
  `>`. Hold 0 leaves coils on. An unsigned maximum (legacy `-1`) is also treated
  as a hold that never expires.
- `StepperModule` claims its pins, converts the template periods into the clock
  units, and calls `iterate()` once per `runLoop()`.

The unused `setl_cyclic_int.h` include is not carried forward. The custom-sequence
constructor now takes typed periods; the original `long` parameters could not bind
to an explicit `Period` constructor.

## Extraction corrections and evidence

`ardo::CoreIF::now()` / `MillisTime` are replaced by `Clock`. Output pins replace
implicit Arduino pins. Decoding tables, bit order, wrap, coil-off and float
scaling are otherwise preserved. The old combined encoder harness is an optional
host case linked only when Grevir Encoder is present; it is not a production
dependency. The Arduino `StepperEncoder` example remains planned.

Native Catch2 cases cover pin masks, strict period expiry, four-phase wrap,
reverse, coil off vs hold, time scale, target/remaining arithmetic, idle
scheduling, a custom sequence, the historical module loop and two independent
modules. Two public headers compile independently. Two valid and two rejected
pin-claim probes pass. Isolated production/install/consumer checks run with
Catch2 and Test Support discovery disabled. AVR compiler and hardware validation
remain on hold.

Standalone host validation can use installed dependencies:

```sh
cmake -S . -B build/host -DCMAKE_PREFIX_PATH=/path/to/grevir/install \
  -DGREVIR_BUILD_HOST_TESTS=ON -DGREVIR_BUILD_COMPILE_CHECKS=ON \
  -DGREVIR_CATCH2_SOURCE_DIR=/path/to/Catch2-3.8.1
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Pulse IO remains a later third-party-free driver. FastLED stays held.
