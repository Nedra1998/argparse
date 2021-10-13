#pragma once

#include <algorithm>
#include <any>
#include <cstring>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#define ARGPARSE_VERSION_MAJOR 0
#define ARGPARSE_VERSION_MINOR 1
#define ARGPARSE_VERSION_PATCH 0

namespace argparse {

namespace detail {
static const std::basic_regex<char> truthy_pattern("(t|T)(rue)?|1");
static const std::basic_regex<char> falsy_pattern("(f|F)(else)?|0");
static const std::basic_regex<char>
    integer_pattern("(-)?(0x|0X|0b|0)?([0-9a-zA-Z]+)|((0x|0X|0b)?0)");

#if defined(_MSC_VER) && !defined(__clang__)
template <typename T> struct identity { using type = T; };
#else
template <typename T> using identity = T;
#endif

template <typename... T> constexpr std::string_view n() noexcept {
#if defined(__clang__)
  return {__PRETTY_FUNCTION__ + 45, sizeof(__PRETTY_FUNCTION__) - 48};
#elif defined(__GNUC__)
  return {__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__)};
#elif defined(_MSC_VER)
  return {__FUNCSIG__, sizeof(__FUNCSIG__)};
#endif
}

template <typename... T> inline constexpr auto type_name_v = n<T...>();

template <typename T>
[[nodiscard]] constexpr std::string_view nameof() noexcept {
  using U = identity<std::remove_cv_t<std::remove_reference_t<T>>>;
  constexpr std::string_view name = type_name_v<U>;
  static_assert(name.size() > 0, "Type does not have a name.");
  return name;
}
} // namespace detail

class Exception : public std::exception {
public:
  explicit Exception(std::string message) : message_(std::move(message)) {}

  [[nodiscard]] const char *what() const noexcept override {
    return message_.c_str();
  }

private:
  std::string message_;
};

class SpecException : public Exception {
public:
  explicit SpecException(const std::string &message) : Exception(message) {}
};

class ParseException : public Exception {
public:
  explicit ParseException(const std::string &message) : Exception(message) {}
};

class argument_incorrect_type : public ParseException {
public:
  explicit argument_incorrect_type(const std::string &arg)
      : ParseException("Argument '" + arg + "' failed to parse") {}
  explicit argument_incorrect_type(const std::string &arg,
                                   const std::string_view &type)
      : ParseException("Argument '" + arg + "' failed to parse as type '" +
                       std::string{type} + "'") {}
};

// TODO: Change all of these template value containers, into using std::any, as
// that provides runtime exceptions if the type is incorrect, which is better
// than implicitly creating one.

class Value {
public:
  template <typename T>
  Value(const T &value, std::uint8_t count = 0)
      : value_{std::move(value)}, count_{std::move(count)} {}

  template <typename T> inline T as() const { return std::any_cast<T>(value_); }
  inline std::uint8_t count() const { return count_; }
  inline const std::any get() const { return value_; }

  inline operator bool() const {
    if (!value_.has_value())
      return false;
    else if (value_.type() == typeid(bool))
      return std::any_cast<bool>(value_);
    return true;
  }

  template <typename T>
  inline auto operator==(const T &rhs) const -> decltype(rhs == rhs) {
    return std::any_cast<T>(value_) == rhs;
  }
  template <typename T>
  inline auto operator!=(const T &rhs) const -> decltype(rhs != rhs) {
    return std::any_cast<T>(value_) != rhs;
  }
  template <typename T>
  inline auto operator<(const T &rhs) const -> decltype(rhs < rhs) {
    return std::any_cast<T>(value_) < rhs;
  }
  template <typename T>
  inline auto operator>(const T &rhs) const -> decltype(rhs > rhs) {
    return std::any_cast<T>(value_) > rhs;
  }
  template <typename T>
  inline auto operator<=(const T &rhs) const -> decltype(rhs <= rhs) {
    return std::any_cast<T>(value_) <= rhs;
  }
  template <typename T>
  inline auto operator>=(const T &rhs) const -> decltype(rhs >= rhs) {
    return std::any_cast<T>(value_) >= rhs;
  }

protected:
  std::any value_;
  std::uint8_t count_;
};

class Result {
public:
  template <typename T> inline T get(std::string_view key) const {
    return data_.at(key).as<T>();
  }
  inline const Value &operator[](std::string_view key) const {
    return data_.at(key);
  }
  inline std::uint8_t count(std::string_view key) const {
    return data_.at(key).count();
  }
  inline bool has(std::string_view key) const {
    return data_.find(key) != data_.end();
  }

  inline void insert(std::string_view key, Value value) {
    data_.insert({key, std::move(value)});
  }

  template <typename T>
  inline void insert(std::string_view key, const T &value) {
    data_.insert({key, Value{std::move(value)}});
  }

private:
  std::unordered_map<std::string_view, Value> data_;
};

template <typename T,
          typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
inline void parse(const std::string &text, T &value) {

  using US = typename std::make_unsigned<T>::type;

  std::smatch match;
  std::regex_match(text, match, detail::integer_pattern);
  if (match.length() == 0)
    throw argument_incorrect_type{text, detail::nameof<T>()};

  const bool negative = match[1].length() > 0;

  std::uint8_t base = 10;
  if (match[2] == "0x" || match[2] == "0X")
    base = 16;
  else if (match[2] == "0")
    base = 8;
  else if (match[2] == "0b")
    base = 2;

  const std::string &val = match[3];

  US result = 0;
  for (char ch : val) {
    US digit = 0;
    if (base == 2) {
      if (ch == '0' || ch == '1')
        digit = static_cast<US>(ch - '0');
      else
        throw argument_incorrect_type{text, detail::nameof<T>()};
    } else if (base == 8) {
      if (ch >= '0' && ch <= '7')
        digit = static_cast<US>(ch - '0');
      else
        throw argument_incorrect_type{text, detail::nameof<T>()};
    } else if (base == 10) {
      if (ch >= '0' && ch <= '9')
        digit = static_cast<US>(ch - '0');
      else
        throw argument_incorrect_type{text, detail::nameof<T>()};
    } else if (base == 16) {
      if (ch >= '0' && ch <= '9')
        digit = static_cast<US>(ch - '0');
      else if (ch >= 'a' && ch <= 'f')
        digit = static_cast<US>(ch - 'a' + 10);
      else if (ch >= 'A' && ch <= 'F')
        digit = static_cast<US>(ch - 'A' + 10);
      else
        throw argument_incorrect_type{text, detail::nameof<T>()};
    } else {
      throw argument_incorrect_type{text, detail::nameof<T>()};
    }

    const US next = static_cast<US>(result * base + digit);
    if (result > next)
      throw argument_incorrect_type{text, detail::nameof<T>()};
    else if ((std::numeric_limits<US>::max() - digit) / base < result)
      throw argument_incorrect_type{text, detail::nameof<T>()};
    result = next;
  }

  if constexpr (std::numeric_limits<T>::is_signed) {
    if (negative) {
      if (result > static_cast<US>(std::numeric_limits<T>::min()))
        throw argument_incorrect_type{text, detail::nameof<T>()};
    } else {
      if (result > static_cast<US>(std::numeric_limits<T>::max()))
        throw argument_incorrect_type{text, detail::nameof<T>()};
    }
  }

  if (negative) {
    if constexpr (std::numeric_limits<T>::is_signed) {
      value = static_cast<T>(-static_cast<T>(result - 1) - 1);
    } else {
      throw argument_incorrect_type{text, detail::nameof<T>()};
    }
  } else {
    value = static_cast<T>(result);
  }
}

inline void parse(const std::string &text, bool &value) {
  std::smatch result;
  std::regex_match(text, result, detail::truthy_pattern);
  if (!result.empty()) {
    value = true;
  } else {
    std::regex_match(text, result, detail::falsy_pattern);
    if (!result.empty()) {
      value = false;
    } else {
      throw argument_incorrect_type{text, detail::nameof<bool>()};
    }
  }
}

inline void parse(const std::string &text, std::string &value) { value = text; }

inline void parse(const std::string &text, char &c) {
  if (text.length() != 1)
    throw argument_incorrect_type{text, detail::nameof<char>()};
  c = text[0];
}

template <typename T, typename std::enable_if<!std::is_integral<T>::value>::type
                          * = nullptr>
inline void parse(const std::string &text, T &value) {
  std::stringstream in(text);
  in >> value;
  if (!in) {
    throw argument_incorrect_type{text, detail::nameof<T>()};
  }
}

template <typename T>
inline void parse(const std::string &text, std::vector<T> &value) {
  if (!text.empty()) {
    std::stringstream in(text);
    std::string token;
    while (!in.eof() && std::getline(in, token, ',')) {
      T v;
      parse(token, v);
      value.emplace_back(std::move(v));
    }
  }
}

template <typename T>
inline void parse(const std::string &text, std::optional<T> &value) {
  if (!text.empty()) {
    T result;
    parse(text, result);
    value = std::move(result);
  }
}

template <typename T> inline T parse(const std::string &text) {
  T res{};
  parse(text, res);
  return res;
}

} // namespace argparse
