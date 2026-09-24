# mqtt-topic

Does an MQTT topic filter match a topic name — per the specification, at compile time if you like.

Part of [hwlib](https://github.com/integra-lib) — architecture-independent C++20
components shared between firmware projects. Header-only,
no exceptions, no RTTI.

## Use it

```bash
git submodule add git@github.com:integra-lib/mqtt-topic.git external/hwlib/mqtt-topic
```

```cmake
add_subdirectory(external/hwlib/mqtt-topic)
target_link_libraries(app PRIVATE Hwlib::mqtt_topic)
```

```cpp
#include <hwlib/communication/mqtt_topic.hpp>
```

Each component carries its own include directory, so this header stays unreachable
until the component is linked: a forgotten dependency is a compile error rather than
a build that happens to work.

## What it does

```cpp
static_assert(hwlib::communication::MatchTopic("home/+/temp", "home/kitchen/temp"));

if (hwlib::communication::MatchTopic(subscription.filter, message.topic))
{
    subscription.handler(message);
}
```

MQTT 3.1.1 section 4.7 (unchanged in 5.0), walked level by level, `/` to `/`:

* `+` matches exactly one level, including an empty one;
* `#` matches the level it stands on, every level below, and the parent level too —
  `sport/#` matches `sport`;
* neither matches a topic that starts with `$` from the first level, so `#` does not
  deliver `$SYS/...` to a client that did not ask for it by name.

A wildcard only counts as one when it is the whole level. A `#` that is not the last
level makes the filter invalid, and an invalid filter matches nothing.

## Coming from a138-ble-gateway's mqtt-helper

`MatchTopic` is the architecture-independent part of a138-ble-gateway's
`esp32/components/mqtt-helper`. The rest of that header is a client over ESP-IDF's
`esp_mqtt_client`, and stays in the project with it.

The function is rewritten, because it had two defects, both confirmed by running the
original:

* **It compared a `#` filter as a string prefix.** `sensors/#` matched
  `sensorsX/temp`, and `a/#` matched `ab`: a subscription could receive topics from a
  neighbouring tree whose name happened to start with the same characters.
* **It could not match `#` against the parent level after a `+`.** `+/#` missed `a`.

All 32 `static_assert` cases the original carried — most of them the specification's
own examples — are kept in the tests unchanged, so the rewrite is held to every
behaviour the original got right.

## Versioning

Every component is released on its own, tagged `vX.Y.Z`. Pre-1.0, a minor release may
break the API, which is why dependants accept a single minor.

```bash
git -C external/hwlib/mqtt-topic fetch --tags
git -C external/hwlib/mqtt-topic checkout v0.2.0
git add external/hwlib/mqtt-topic && git commit -m "build: bump mqtt-topic to v0.2.0"
```

## In a consumer's CI

The component is an ordinary submodule, so the build needs it checked out. On GitLab
that means `GIT_SUBMODULE_STRATEGY: normal` (or `recursive`) on every job that builds —
not only on the ones that run unit tests.

## Develop it

```bash
git submodule update --init          # ci-shared, needed by pre-commit
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
```

Tests are built only when this repository is the top-level project, so a consumer
never builds them and never fetches GoogleTest.

The style configs are symlinks into the `ci-shared` submodule, and the pipeline comes
from the same place. On GitHub this repository carries a self-contained build-and-test
workflow instead: a workflow token cannot read another private repository, so neither
a shared workflow nor the submodule is reachable there. The shared setup is what
GitLab will use.
