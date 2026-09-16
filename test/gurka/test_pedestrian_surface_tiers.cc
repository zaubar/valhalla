#include "gurka.h"

#include <gtest/gtest.h>

using namespace valhalla;

// Three ways from A to D: the direct one is dressed cobblestone in bad repair, the way over B
// and C is sound sett, the way over E and F is asphalt. The parser files sett with smoothness
// bad or worse one class below paved_rough, and the pedestrian costing charges that tier with
// avoid_very_rough_surfaces, so a walker who accepts some paving but not bumpy cobblestone
// gets the sett way, while "smooth only" gets the asphalt detour.
class PedestrianSurfaceTiers : public ::testing::Test {
protected:
  static gurka::map map;

  static void SetUpTestSuite() {
    constexpr double gridsize = 10;

    const std::string ascii_map = R"(
      B--------C
      |        |
      A--------D
      |        |
      |        |
      E--------F
    )";

    const gurka::ways ways = {
        {"AD", {{"highway", "footway"}, {"surface", "sett"}, {"smoothness", "bad"}}},
        {"AB", {{"highway", "footway"}, {"surface", "sett"}}},
        {"BC", {{"highway", "footway"}, {"surface", "sett"}}},
        {"CD", {{"highway", "footway"}, {"surface", "sett"}}},
        {"AE", {{"highway", "footway"}, {"surface", "asphalt"}}},
        {"EF", {{"highway", "footway"}, {"surface", "asphalt"}}},
        {"FD", {{"highway", "footway"}, {"surface", "asphalt"}}},
    };

    const auto layout = gurka::detail::map_to_coordinates(ascii_map, gridsize);
    map = gurka::buildtiles(layout, ways, {}, {}, "test/data/pedestrian_surface_tiers");
  }
};

gurka::map PedestrianSurfaceTiers::map = {};

TEST_F(PedestrianSurfaceTiers, NoPreferenceTakesTheShortWay) {
  auto route = gurka::do_action(Options::route, map, {"A", "D"}, "pedestrian");
  gurka::assert::raw::expect_path(route, {"AD"});
}

TEST_F(PedestrianSurfaceTiers, BadSurfacesAloneCoverBothTiers) {
  std::unordered_map<std::string, std::string> options = {
      {"/costing_options/pedestrian/avoid_bad_surfaces", "0.4"}};
  auto route = gurka::do_action(Options::route, map, {"A", "D"}, "pedestrian", options);
  gurka::assert::raw::expect_path(route, {"AE", "EF", "FD"});
}

TEST_F(PedestrianSurfaceTiers, VeryRoughAloneKeepsSoundSett) {
  std::unordered_map<std::string, std::string> options = {
      {"/costing_options/pedestrian/avoid_very_rough_surfaces", "0.4"}};
  auto route = gurka::do_action(Options::route, map, {"A", "D"}, "pedestrian", options);
  gurka::assert::raw::expect_path(route, {"AB", "BC", "CD"});
}

TEST_F(PedestrianSurfaceTiers, GradedRequestSeparatesTheTiers) {
  std::unordered_map<std::string, std::string> options = {
      {"/costing_options/pedestrian/avoid_bad_surfaces", "0.15"},
      {"/costing_options/pedestrian/avoid_very_rough_surfaces", "1.0"}};
  auto route = gurka::do_action(Options::route, map, {"A", "D"}, "pedestrian", options);
  // sett costs 130 m * 4.6, asphalt 150 m: the bumpy way (90 m * 25) is out, the asphalt detour wins
  gurka::assert::raw::expect_path(route, {"AE", "EF", "FD"});
}
