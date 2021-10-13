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
  res.insert("true", true);
  res.insert("false", false);

  SECTION("boolean cast operator") {
    CHECK(res["integer"]);
    CHECK_THROWS_AS(res["missing"], std::out_of_range);
    CHECK(res["true"]);
    CHECK_FALSE(res["false"]);
  }

  SECTION("has member function") {
    CHECK(res.has("integer"));
    CHECK_FALSE(res.has("missing"));
    CHECK(res.has("true"));
    CHECK(res.has("false"));
  }
}

TEMPLATE_TEST_CASE("result comparison casts", "", bool, char, int, unsigned int,
                   float, double) {
  Result res;
  res.insert<TestType>("a", 0);
  res.insert<TestType>("b", 1);

  SECTION("equality comparison casts") {
    if constexpr (!std::is_floating_point<TestType>::value) {
      CHECK(res["a"] == TestType{0});
      CHECK(res["b"] == TestType{1});
    }
  }

  SECTION("not equal comparison casts") {
    if constexpr (!std::is_floating_point<TestType>::value) {
      CHECK_FALSE(res["a"] != TestType{0});
      CHECK_FALSE(res["b"] != TestType{1});
    }
  }

  SECTION("less-than comparison casts") {
    CHECK_FALSE(res["a"] < TestType{0});
    CHECK_FALSE(res["b"] < TestType{1});
  }

  SECTION("less-than or equal comparison casts") {
    CHECK(res["a"] <= TestType{0});
    CHECK(res["b"] <= TestType{1});
  }

  SECTION("greater-than comparison casts") {
    CHECK_FALSE(res["a"] > TestType{0});
    CHECK_FALSE(res["b"] > TestType{1});
  }

  SECTION("greater-than or equal comparison casts") {
    CHECK(res["a"] >= TestType{0});
    CHECK(res["b"] >= TestType{1});
  }
}

TEMPLATE_TEST_CASE("result explicit casts", "", bool, char, int, unsigned int,
                   float, double) {
  Result res;
  res.insert<TestType>("a", 0);
  res.insert<TestType>("b", 1);

  SECTION("explicit cast to expected type") {
    if constexpr (std::is_floating_point<TestType>::value) {
      CHECK(res["a"].as<TestType>() == Approx(TestType{0}));
      CHECK(res["b"].as<TestType>() == Approx(TestType{1}));
      CHECK_THROWS_AS(res["c"].as<TestType>(), std::out_of_range);
      CHECK(res.get<TestType>("a") == Approx(TestType{0}));
      CHECK(res.get<TestType>("b") == Approx(TestType{1}));
      CHECK_THROWS_AS(res.get<TestType>("c"), std::out_of_range);
    } else {
      CHECK(res["a"].as<TestType>() == TestType{0});
      CHECK(res["b"].as<TestType>() == TestType{1});
      CHECK_THROWS_AS(res["c"].as<TestType>(), std::out_of_range);
      CHECK(res.get<TestType>("a") == TestType{0});
      CHECK(res.get<TestType>("b") == TestType{1});
      CHECK_THROWS_AS(res.get<TestType>("c"), std::out_of_range);
    }
  }

  SECTION("explicit cast to invalid type") {
    CHECK_THROWS_AS(res["a"].as<std::string>(), std::bad_any_cast);
    CHECK_THROWS_AS(res["b"].as<std::string>(), std::bad_any_cast);
    CHECK_THROWS_AS(res["c"].as<std::string>(), std::out_of_range);
    CHECK_THROWS_AS(res.get<std::string>("a"), std::bad_any_cast);
    CHECK_THROWS_AS(res.get<std::string>("b"), std::bad_any_cast);
    CHECK_THROWS_AS(res.get<std::string>("c"), std::out_of_range);
  }
}
