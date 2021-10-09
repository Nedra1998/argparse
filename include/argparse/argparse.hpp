#pragma once

#include <algorithm>
#include <memory>
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

class ValueContainer;

class ValueBase {
public:
  ValueBase(const std::uint8_t &count) : count_(count) {}

protected:
  friend class ValueContainer;
  std::uint8_t count_;
};

template <typename T> class Value : public ValueBase {
public:
  Value(const T &value) : ValueBase(0), value_(value){};

  template <typename U = T>
  inline typename std::enable_if<std::is_convertible<T, U>::value, U>::type
  as() const {
    return static_cast<U>(value_);
  }

protected:
  friend class ValueContainer;
  T value_;
};

class ValueContainer {
public:
  std::shared_ptr<ValueBase> ptr;

  template <typename T, typename U = T>
  inline typename std::enable_if<std::is_convertible<T, U>::value, U>::type
  as() const {
    if (ptr != nullptr)
      return static_cast<U>(
          std::static_pointer_cast<const Value<T>>(ptr)->value_);
    return U{};
  }
  inline bool has() const { return ptr != nullptr; }
  inline std::uint8_t count() const { return ptr->count_; }

  template <typename T>
  inline const std::shared_ptr<const Value<T>> get() const {
    return std::static_pointer_cast<const Value<T>>(ptr);
  }

  inline operator bool() const { return ptr != nullptr; }

  template <typename T>
  inline auto operator==(const T &rhs) const -> decltype(rhs == rhs) {
    if (ptr != nullptr)
      return std::static_pointer_cast<const Value<T>>(ptr)->value_ == rhs;
    return false;
  }
  template <typename T>
  inline auto operator!=(const T &rhs) const -> decltype(rhs != rhs) {
    if (ptr != nullptr)
      return std::static_pointer_cast<const Value<T>>(ptr)->value_ != rhs;
    return true;
  }

  template <typename T>
  inline auto operator<(const T &rhs) const -> decltype(rhs < rhs) {
    if (ptr != nullptr)
      return std::static_pointer_cast<const Value<T>>(ptr)->value_ < rhs;
    return false;
  }
  template <typename T>
  inline auto operator>(const T &rhs) const -> decltype(rhs > rhs) {
    if (ptr != nullptr)
      return std::static_pointer_cast<const Value<T>>(ptr)->value_ > rhs;
    return false;
  }
  template <typename T>
  inline auto operator<=(const T &rhs) const -> decltype(rhs <= rhs) {
    if (ptr != nullptr)
      return std::static_pointer_cast<const Value<T>>(ptr)->value_ <= rhs;
    return false;
  }
  template <typename T>
  inline auto operator>=(const T &rhs) const -> decltype(rhs >= rhs) {
    if (ptr != nullptr)
      return std::static_pointer_cast<const Value<T>>(ptr)->value_ >= rhs;
    return false;
  }
};

class Result {
public:
  template <typename T, typename U = T>
  inline typename std::enable_if<std::is_convertible<T, U>::value, U>::type
  get(const std::string_view &key) const {
    const std::shared_ptr<const Value<T>> ptr = data_.at(key).get<T>();
    if (ptr != nullptr)
      return ptr->template as<U>();
    return U{};
  }

  inline const ValueContainer &operator[](const std::string_view &key) const {
    return data_.at(key);
  }

  inline std::uint8_t count(const std::string_view &key) const {
    return data_.at(key).count();
  }
  inline bool has(const std::string_view &key) const {
    return data_.at(key).has();
  }

  inline void insert(const std::string_view &key, const ValueContainer &val) {
    data_.insert({key, val});
  }
  template <typename T>
  inline void insert(const std::string_view &key, const T &val) {
    data_.insert({key, ValueContainer{std::make_shared<Value<T>>(val)}});
  }
  template <>
  inline void insert(const std::string_view &key, const std::nullptr_t &) {
    data_.insert({key, ValueContainer{nullptr}});
  }

protected:
  std::unordered_map<std::string_view, ValueContainer> data_;
};

} // namespace argparse
