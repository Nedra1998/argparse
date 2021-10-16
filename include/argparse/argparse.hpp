#pragma once

#include <algorithm>
#include <any>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#if defined(__has_include)
#  if __has_include(<fmt/format.h>)
#    define ARGPARSE_USE_FMT
#    include <fmt/chrono.h>
#    include <fmt/compile.h>
#    include <fmt/core.h>
#    include <fmt/format.h>
#    include <fmt/ostream.h>
#    include <fmt/ranges.h>
#  endif
#endif

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

template <typename T, typename Char> class is_streamable {
private:
  template <typename U>
  static auto test(int)
      -> std::bool_constant<sizeof(std::declval<std::basic_ostream<Char> &>()
                                   << std::declval<U>()) != 0>;

  template <typename> static auto test(...) -> std::false_type;

  using result = decltype(test<T>(0));

public:
  is_streamable() = default;

  static const bool value = result::value;
};

template <typename T, typename _ = void>
struct is_container : std::false_type {};

template <typename... Ts> struct is_container_helper {};

template <typename T>
struct is_container<
    T, std::conditional_t<
           false,
           is_container_helper<typename T::value_type, typename T::size_type,
                               typename T::allocator_type, typename T::iterator,
                               typename T::const_iterator,
                               decltype(std::declval<T>().size()),
                               decltype(std::declval<T>().begin()),
                               decltype(std::declval<T>().end()),
                               decltype(std::declval<T>().cbegin()),
                               decltype(std::declval<T>().cend())>,
           void>> : public std::true_type {};

inline std::string wrap(std::string_view text, std::uint8_t indent = 0,
                        std::uint8_t init_width = 0, std::uint8_t width = 80) {
  std::size_t line_length = init_width;
  std::string result;

  std::string prefix = "\n" + std::string(indent, ' ');

  std::size_t prev = 0, pos = 0;
  while ((pos = text.find(' ', prev + 1)) != std::string_view::npos) {
    if (line_length + (pos - prev) > width) {
      result += prefix;
      result += std::string{text.substr(prev + 1, (pos - prev) - 1)};
      line_length = (pos - prev) - 1 + prefix.length() - 1;
    } else {
      result += std::string{text.substr(prev, pos - prev)};
      line_length += (pos - prev);
    }
    prev = pos;
  }

  if (line_length + (text.length() - prev) > width) {
    result += prefix;
    result += std::string{text.substr(prev + 1, (text.length() - prev) - 1)};
  } else {
    result += std::string{text.substr(prev, text.length() - prev)};
  }

  return result;
}

inline std::string pad_right(std::string_view text, std::uint8_t width) {
  if (text.length() >= width)
    return std::string{text};
  return std::string{text} + std::string(width - text.length(), ' ');
}

inline bool is_positional(std::string_view name) {
  if (name.empty())
    return false;
  else if (name[0] == '-')
    return false;
  else
    return true;
}

#ifdef ARGPARSE_USE_FMT
template <typename T> std::string to_string(const T &val) {
  return fmt::format(FMT_COMPILE("{}"), val);
}
#else
template <typename T>
typename std::enable_if<is_streamable<T, char>::value, std::string>::type
to_string(const T &val) {
  if constexpr (std::is_same<T, std::uint8_t>::value) {
    std::stringstream out;
    out << static_cast<int>(val);
    return out.str();
  } else {
    std::stringstream out;
    out << val;
    return out.str();
  }
}
template <typename T>
typename std::enable_if<!is_streamable<T, char>::value, std::string>::type
to_string(const T &) {
  return std::string();
}
#endif
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

class ArgumentParser;

class ArgumentBase {
public:
  virtual ~ArgumentBase() = default;

  virtual std::string get_usage() const {
    std::string res;
    std::string meta =
        names_.back().substr(names_.back().find_first_not_of('-'));
    std::transform(meta.begin(), meta.end(), meta.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    if (!is_positional_) {
      res = names_.back();
    }

    if (nargs_ > 0) {
      for (std::uint8_t i = 0; i < nargs_; ++i)
        res += " " + meta;
    } else if (nargs_ == -1) {
      res += " [" + meta + "]";
    } else if (nargs_ == -2) {
      res += " " + meta + " [" + meta + " ...]";
    } else if (nargs_ == -3) {
      res += " [" + meta + " [" + meta + " ...]]";
    }

    if (!is_required_)
      res = "[" + res + "]";
    if (res[0] == ' ')
      res.erase(res.begin());
    return res;
  }

  virtual std::string get_help_key() const {
    if (description_.empty())
      return "";
    std::string res;
    if (!is_positional_) {
      for (std::size_t i = 0; i < names_.size(); ++i) {
        res += names_[i];
        if (i != names_.size() - 1)
          res += ", ";
      }
    }
    if (nargs_ != 0) {
      std::string meta =
          names_.back().substr(names_.back().find_first_not_of('-'));
      std::transform(meta.begin(), meta.end(), meta.begin(),
                     [](unsigned char c) { return std::toupper(c); });
      res += " " + meta;
    }
    return res;
  }

  virtual std::string get_help() const { return description_; }

  virtual ArgumentBase &group(std::string group) {
    group_ = std::move(group);
    return *this;
  }
  virtual ArgumentBase &help(std::string help) {
    description_ = std::move(help);
    return *this;
  }
  virtual ArgumentBase &description(std::string description) {
    description_ = std::move(description);
    return *this;
  }
  virtual ArgumentBase &nargs(std::int8_t nargs) {
    if (nargs >= '0' && nargs <= '9')
      nargs_ = nargs - '0';
    else if (nargs == '?')
      nargs_ = -1;
    else if (nargs == '+')
      nargs_ = -2;
    else if (nargs == '*')
      nargs_ = -3;
    nargs_ = nargs;
    return *this;
  }
  virtual ArgumentBase &required(bool required = true) {
    is_required_ = required;
    return *this;
  }
  virtual ArgumentBase &optional(bool optional = true) {
    is_required_ = !optional;
    return *this;
  }

protected:
  friend class ArgumentParser;

  template <std::size_t N, std::size_t... I>
  explicit ArgumentBase(std::string_view(&&a)[N], std::index_sequence<I...>)
      : names_{}, is_positional_((detail::is_positional(a[I]) || ...)),
        is_required_(false), nargs_(1), group_{} {
    ((void)names_.emplace_back(a[I]), ...);
    std::sort(
        names_.begin(), names_.end(), [](const auto &lhs, const auto &rhs) {
          return lhs.size() == rhs.size() ? lhs < rhs : lhs.size() < rhs.size();
        });
    if (is_positional_) {
      is_required_ = true;
      group_       = "Positional";
    }
  }
  std::vector<std::string> names_;
  bool is_positional_, is_required_;
  std::int8_t nargs_;
  std::string group_, description_;
};

template <typename T> class Argument : public ArgumentBase {
public:
  template <std::size_t N>
  explicit Argument(std::string_view(&&a)[N])
      : ArgumentBase(std::move(a), std::make_index_sequence<N>{}) {
    if constexpr (std::is_same<T, bool>::value) {
      nargs_    = 0;
      default_  = false;
      implicit_ = true;
    } else if constexpr (detail::is_container<T>::value) {
      nargs_ = -2;
    }
  }

  Argument<T> &group(std::string group) override {
    group_ = std::move(group);
    return *this;
  }
  Argument<T> &help(std::string help) override {
    description_ = std::move(help);
    return *this;
  }
  Argument<T> &description(std::string description) override {
    description_ = std::move(description);
    return *this;
  }
  Argument<T> &nargs(std::int8_t nargs) override {
    if (nargs >= '0' && nargs <= '9')
      nargs_ = nargs - '0';
    else if (nargs == '?')
      nargs_ = -1;
    else if (nargs == '+')
      nargs_ = -2;
    else if (nargs == '*')
      nargs_ = -3;
    nargs_ = nargs;
    return *this;
  }
  Argument<T> &required(bool required = true) override {
    is_required_ = required;
    return *this;
  }
  Argument<T> &optional(bool optional = true) override {
    is_required_ = !optional;
    return *this;
  }

  template <typename U>
  typename std::enable_if<std::is_convertible<U, T>::value, Argument<T>>::type &
  default_value(const U &value) {
    default_ = static_cast<T>(value);
    return *this;
  }

  template <typename U>
  typename std::enable_if<std::is_convertible<U, T>::value, Argument<T>>::type &
  implicit_value(const U &value) {
    implicit_ = static_cast<T>(value);
    return *this;
  }

  std::string get_help() const override {
    std::string res = description_;
    if constexpr (std::is_same<T, bool>::value) {
      if (nargs_ != 0) {
        res += " [" + std::string{detail::nameof<bool>()};
        if (default_.has_value())
          res += "=" + detail::to_string(default_.value());
        if (implicit_.has_value())
          res += "(=" + detail::to_string(implicit_.value()) + ")";
        res += "]";
      }
    } else {
      res += " [" + std::string{detail::nameof<T>()};
      if (default_.has_value())
        res += "=" + detail::to_string(default_.value());
      if (implicit_.has_value())
        res += "(=" + detail::to_string(implicit_.value()) + ")";
      res += "]";
    }
    return res;
  }

protected:
  std::optional<T> default_, implicit_;
};

class ArgumentParser {
public:
  ArgumentParser(std::string program_name = {})
      : short_usage_(false), program_name_{std::move(program_name)} {}

  ArgumentParser(ArgumentParser &&) noexcept = default;
  ArgumentParser &operator=(ArgumentParser &&) = default;

  ArgumentParser &description(std::string description) {
    description_ = std::move(description);
    return *this;
  }
  ArgumentParser &short_description(std::string short_description) {
    short_ = std::move(short_description);
    return *this;
  }
  ArgumentParser &epilog(std::string epilog) {
    epilog_ = std::move(epilog);
    return *this;
  }

  ArgumentParser &add_subcommand(const std::string &name,
                                 const std::string &help) {
    std::shared_ptr<ArgumentParser> subcommand =
        std::make_shared<ArgumentParser>(name);
    subcommand->short_description(help);
    subcommands_.push_back(subcommand);
    sort_subcommands();
    return *subcommand;
  }

  template <typename T = bool, typename... Args>
  Argument<T> &add_argument(Args... args) {
    using array_of_sv = std::string_view[sizeof...(Args)];
    std::shared_ptr<Argument<T>> argument =
        std::make_shared<Argument<T>>(array_of_sv{args...});
    arguments_.push_back(std::static_pointer_cast<ArgumentBase>(argument));
    sort_arguments();
    if (argument->is_positional_) {
      positional_.push_back(std::static_pointer_cast<ArgumentBase>(argument));
    }
    return *argument;
  }

  std::string usage() const {
    std::string usage_msg = "Usage: " + program_name_;

    if (short_usage_) {
      if (!arguments_.empty())
        usage_msg += " [options]";

      for (auto &it : positional_)
        usage_msg += " " + it->get_usage();
    } else {
      for (auto &it : arguments_)
        if (!it->is_positional_)
          usage_msg += " " + it->get_usage();
      for (auto &it : positional_)
        usage_msg += " " + it->get_usage();
    }

    if (!subcommands_.empty())
      usage_msg += " [subcommand...]";

    return detail::wrap(usage_msg,
                        8 + static_cast<std::uint8_t>(program_name_.length()));
  }

  std::string help() const {
    std::string help_msg = usage() + "\n\n";

    if (!short_.empty())
      help_msg += detail::wrap(short_) + "\n\n";

    if (!description_.empty())
      help_msg += detail::wrap(description_) + "\n\n";

    if (!subcommands_.empty()) {
      help_msg += "Subcommands:\n";

      std::uint8_t longest_cmd = 0;
      for (auto &it : subcommands_)
        longest_cmd = std::max(
            longest_cmd, static_cast<std::uint8_t>(it->program_name_.size()));

      for (auto &it : subcommands_)
        help_msg +=
            "  " + detail::pad_right(it->program_name_, longest_cmd) + "  " +
            detail::wrap(it->short_, longest_cmd + 4, longest_cmd + 4) + "\n";

      help_msg += "\n";
    }

    if (!arguments_.empty()) {
      std::set<std::string> groups;
      for (const auto &it : arguments_)
        if (!it->group_.empty())
          groups.insert(it->group_);

      std::uint8_t longest_key = 0;
      for (const auto &it : arguments_)
        longest_key = std::max(
            longest_key, static_cast<std::uint8_t>(it->get_help_key().size()));

      if (std::any_of(arguments_.begin(), arguments_.end(), [](const auto &v) {
            return v->group_.empty() && !v->description_.empty();
          })) {
        help_msg += "Options:\n";
        for (const auto &it : arguments_) {
          if (!it->group_.empty() || it->description_.empty())
            continue;
          help_msg +=
              "  " + detail::pad_right(it->get_help_key(), longest_key) + "  " +
              detail::wrap(it->get_help(), longest_key + 4, longest_key + 4) +
              "\n";
        }
        help_msg += "\n";
      }

      for (const auto &group : groups) {
        if (std::any_of(arguments_.begin(), arguments_.end(),
                        [group](const auto &v) {
                          return v->group_ == group && !v->description_.empty();
                        })) {
          help_msg += group + ":\n";
          for (const auto &it : arguments_) {
            if (it->group_ != group || it->description_.empty())
              continue;
            help_msg +=
                "  " + detail::pad_right(it->get_help_key(), longest_key) +
                "  " +
                detail::wrap(it->get_help(), longest_key + 4, longest_key + 4) +
                "\n";
          }
          help_msg += "\n";
        }
      }
    }

    if (!epilog_.empty())
      help_msg += detail::wrap(epilog_) + "\n\n";
    help_msg.pop_back();
    return help_msg;
  }

protected:
  bool short_usage_;
  std::string program_name_;
  std::string short_, description_, epilog_;

  std::vector<std::shared_ptr<ArgumentBase>> arguments_;
  std::vector<std::shared_ptr<ArgumentBase>> positional_;
  std::vector<std::shared_ptr<ArgumentParser>> subcommands_;

private:
  void sort_subcommands() {
    if (!std::is_sorted(subcommands_.begin(), subcommands_.end(),
                        [](const std::shared_ptr<ArgumentParser> &lhs,
                           const std::shared_ptr<ArgumentParser> &rhs) {
                          return lhs->program_name_ < rhs->program_name_;
                        })) {
      std::sort(subcommands_.begin(), subcommands_.end(),
                [](const std::shared_ptr<ArgumentParser> &lhs,
                   const std::shared_ptr<ArgumentParser> &rhs) {
                  return lhs->program_name_ < rhs->program_name_;
                });
    }
  }

  void sort_arguments() {
    if (!std::is_sorted(arguments_.begin(), arguments_.end(),
                        [](const std::shared_ptr<ArgumentBase> &lhs,
                           const std::shared_ptr<ArgumentBase> &rhs) {
                          return lhs->names_.back() < rhs->names_.back();
                        })) {
      std::sort(arguments_.begin(), arguments_.end(),
                [](const std::shared_ptr<ArgumentBase> &lhs,
                   const std::shared_ptr<ArgumentBase> &rhs) {
                  return lhs->names_.back() < rhs->names_.back();
                });
    }
  }
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
