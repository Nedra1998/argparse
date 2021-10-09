#include <catch2/catch_approx.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <argparse/argparse.hpp>

using namespace argparse;
using namespace Catch;
using namespace Catch::literals;

TEST_CASE("result is truthy based on presence") {
  Result res;
  res.insert("integer", -32);
  res.insert("missing", nullptr);
  res.insert("true", true);
  res.insert("false", false);

  SECTION("boolean cast operator") {
    CHECK(res["integer"]);
    CHECK_FALSE(res["missing"]);
    CHECK(res["true"]);
    CHECK(res["false"]);
  }

  SECTION("has member function") {
    CHECK(res["integer"].has());
    CHECK_FALSE(res["missing"].has());
    CHECK(res["true"].has());
    CHECK(res["false"].has());
  }
}

TEMPLATE_TEST_CASE("result comparison casts", "", bool, char, int, unsigned int,
                   float, double) {
  Result res;
  res.insert<TestType>("a", 0);
  res.insert<TestType>("b", 1);
  res.insert("c", nullptr);

  SECTION("equality comparison casts") {
    if constexpr (!std::is_floating_point<TestType>::value) {
      CHECK(res["a"] == TestType{0});
      CHECK(res["b"] == TestType{1});
      CHECK_FALSE(res["c"] == TestType{0});
    }
  }

  SECTION("not equal comparison casts") {
    if constexpr (!std::is_floating_point<TestType>::value) {
      CHECK_FALSE(res["a"] != TestType{0});
      CHECK_FALSE(res["b"] != TestType{1});
      CHECK(res["c"] != TestType{0});
    }
  }

  SECTION("less-than comparison casts") {
    CHECK_FALSE(res["a"] < TestType{0});
    CHECK_FALSE(res["b"] < TestType{1});
    CHECK_FALSE(res["c"] < TestType{0});
  }

  SECTION("less-than or equal comparison casts") {
    CHECK(res["a"] <= TestType{0});
    CHECK(res["b"] <= TestType{1});
    CHECK_FALSE(res["c"] <= TestType{0});
  }

  SECTION("greater-than comparison casts") {
    CHECK_FALSE(res["a"] > TestType{0});
    CHECK_FALSE(res["b"] > TestType{1});
    CHECK_FALSE(res["c"] > TestType{0});
  }

  SECTION("greater-than or equal comparison casts") {
    CHECK(res["a"] >= TestType{0});
    CHECK(res["b"] >= TestType{1});
    CHECK_FALSE(res["c"] >= TestType{0});
  }
}

TEMPLATE_TEST_CASE("result explicit casts", "", bool, char, int, unsigned int,
                   float, double) {
  Result res;
  res.insert<TestType>("a", 0);
  res.insert<TestType>("b", 1);
  res.insert("c", nullptr);

  SECTION("explicit cast to base type") {
    if constexpr (std::is_floating_point<TestType>::value) {
      CHECK(res["a"].as<TestType>() == Approx(TestType{0}));
      CHECK(res["b"].as<TestType>() == Approx(TestType{1}));
      CHECK(res["c"].as<TestType>() == Approx(TestType{0}));
      CHECK(res.get<TestType>("a") == Approx(TestType{0}));
      CHECK(res.get<TestType>("b") == Approx(TestType{1}));
      CHECK(res.get<TestType>("c") == Approx(TestType{0}));
    } else {
      CHECK(res["a"].as<TestType>() == TestType{0});
      CHECK(res["b"].as<TestType>() == TestType{1});
      CHECK(res["c"].as<TestType>() == TestType{0});
      CHECK(res.get<TestType>("a") == TestType{0});
      CHECK(res.get<TestType>("b") == TestType{1});
      CHECK(res.get<TestType>("c") == TestType{0});
    }
  }

  SECTION("explicit cast to convertible type") {
    if constexpr (std::is_floating_point<TestType>::value) {
      CHECK(res["a"].as<TestType, int>() == int{0});
      CHECK(res["b"].as<TestType, int>() == int{1});
      CHECK(res["c"].as<TestType, int>() == int{0});
      CHECK(res.get<TestType, int>("a") == int{0});
      CHECK(res.get<TestType, int>("b") == int{1});
      CHECK(res.get<TestType, int>("c") == int{0});
    } else {
      CHECK(res["a"].as<TestType, float>() == Approx(float{0}));
      CHECK(res["b"].as<TestType, float>() == Approx(float{1}));
      CHECK(res["c"].as<TestType, float>() == Approx(float{0}));
      CHECK(res.get<TestType, float>("a") == Approx(float{0}));
      CHECK(res.get<TestType, float>("b") == Approx(float{1}));
      CHECK(res.get<TestType, float>("c") == Approx(float{0}));
    }
  }
}

/* CHECK(res["integer"] == -32);
CHECK(res["integer"] != 32);
CHECK(res["integer"] > -100);
CHECK(res["integer"] < 0);

CHECK_FALSE(res["missing"] == bool{});
CHECK_FALSE(res["missing"] == char{});
CHECK_FALSE(res["missing"] == std::string{});

CHECK(res["true"] == true);
CHECK(res["true"] != false);
CHECK(res["false"] == false);
CHECK(res["false"] != true);

CHECK(res["integer"].as<int>() == -32);
CHECK(res["integer"].as<int, float>() == -32.0_a);
CHECK(res["missing"].as<int>() == int{});
CHECK(res["true"].as<bool>() == true);
CHECK(res.get<int>("integer") == -32);
CHECK(res.get<int, float>("integer") == -32.0_a);
CHECK(res.get<int>("missing") == int{});
CHECK(res.get<bool>("true") == true);
} */
