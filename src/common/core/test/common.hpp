#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

struct RunListener : public Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const& testRunInfo) override;
};

CATCH_REGISTER_LISTENER(RunListener) // NOLINT
