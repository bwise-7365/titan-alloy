// Copyright Ben Paul Wise. All Rights Reserved.
#include "config.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace VIMCP;
using namespace VIMCP::Network;

// Parsing: comments, blank lines, trimming; malformed input throws.
TEST(NetworkConfig, ParsesTextAndRejectsGarbage) {
    const ConfigEntries entries = parseConfigText(
        "# a comment\n"
        "\n"
        "  solver.magTol = 1.0e-10   # trailing comment\n"
        "benchmark.name = spaced out name\n");
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries.at("solver.magTol"), "1.0e-10");
    EXPECT_EQ(entries.at("benchmark.name"), "spaced out name");

    EXPECT_THROW(parseConfigText("no equals sign here\n"),
                 std::invalid_argument);
    EXPECT_THROW(parseConfigText("= value without key\n"),
                 std::invalid_argument);
    EXPECT_THROW(parseConfigText("key.without.value =\n"),
                 std::invalid_argument);
    EXPECT_THROW(parseConfigText("twice = 1\ntwice = 2\n"),
                 std::invalid_argument);
}

// Typed consumption: applied keys land in the params, absent keys keep
// defaults, junk values and leftover (unknown) keys throw with names.
TEST(NetworkConfig, AppliesSolverAndScreenKeys) {
    ConfigEntries entries = parseConfigText(
        "solver.magTol = 1.0e-10\n"
        "solver.engine = chain\n"
        "solver.roughIterMax = 5000\n"
        "solver.ipmNewton = flow\n"
        "solver.newtonCheckTol = 1.0e-8\n"
        "screen.maxSourcesPerSink = 7\n"
        "screen.gapFraction = 0.05\n"
        "screen.certificateSlack = 1.0e-4\n");
    FlowPlanParams params;
    const int defaultIterMax = params.iterMax;
    applyFlowPlanConfig(entries, params);

    EXPECT_EQ(params.magTol, 1.0e-10);
    EXPECT_EQ(params.engine, "chain");
    EXPECT_EQ(params.roughIterMax, 5000);
    EXPECT_EQ(params.ipmNewton, "flow");
    EXPECT_EQ(params.newtonCheckTol, 1.0e-8);
    EXPECT_EQ(params.maxSourcesPerSink, 7);
    EXPECT_EQ(params.gapFraction, 0.05);
    EXPECT_EQ(params.certificateSlack, 1.0e-4);
    EXPECT_EQ(params.iterMax, defaultIterMax);   // untouched default
    EXPECT_NO_THROW(requireAllConsumed(entries));

    ConfigEntries typo = parseConfigText("solver.magToll = 1.0e-10\n");
    FlowPlanParams ignored;
    applyFlowPlanConfig(typo, ignored);
    EXPECT_THROW(requireAllConsumed(typo), std::invalid_argument);

    ConfigEntries junk = parseConfigText("solver.magTol = fast\n");
    EXPECT_THROW(applyFlowPlanConfig(junk, ignored), std::invalid_argument);
}

// Profile keys reshape the generator; the result must still validate.
TEST(NetworkConfig, AppliesProfileKeys) {
    ConfigEntries entries = parseConfigText(
        "profile.numSupplyOnly = 3\n"
        "profile.numBoth = 2\n"
        "profile.numDemandOnly = 4\n"
        "profile.numNeither = 1\n"
        "profile.laydownType = 1\n"
        "profile.jitterHalfWidth = 0.03\n");
    InstanceProfile profile;
    applyProfileConfig(entries, profile);
    EXPECT_NO_THROW(requireAllConsumed(entries));

    EXPECT_EQ(profile.numSupplyOnly, 3);
    EXPECT_EQ(profile.laydownType, 1);
    EXPECT_EQ(profile.jitterHalfWidth, 0.03);
    EXPECT_NO_THROW(validateProfile(profile));

    const Instance inst = makeRandomInstance(profile, 20260704);
    EXPECT_EQ(inst.numNodes, 10);
}

// File round-trip through the real reader.
TEST(NetworkConfig, ReadsFiles) {
    namespace fs = std::filesystem;
    const fs::path path =
        fs::temp_directory_path() / "vimcp_network_config_test.cfg";
    {
        std::ofstream file(path);
        ASSERT_TRUE(file.good());
        file << "screen.gapFraction = 0.10\n";
    }
    ConfigEntries entries = parseConfigFile(path.string());
    FlowPlanParams params;
    applyFlowPlanConfig(entries, params);
    EXPECT_EQ(params.gapFraction, 0.10);
    std::remove(path.string().c_str());

    EXPECT_THROW(parseConfigFile((path / "nowhere").string()),
                 std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.
