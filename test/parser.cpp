#include <catch2/catch_approx.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <argparse/argparse.hpp>

using namespace argparse;
using namespace Catch;
using namespace Catch::literals;
using namespace Catch::Matchers;

TEST_CASE("parser") {
  SECTION("string") {
    CHECK(parse<std::string>("hello world") == "hello world");
    // CHECK(parse<bool>("hello world") == false);
    CHECK_THAT(parse<std::vector<int>>("1,2,3"), Equals(std::vector<int>{1, 2, 3}));
    CHECK(parse<std::optional<int>>("32") == 32);
    CHECK(parse<std::optional<int>>("") == std::optional<int>{});
  }
}
