#ifndef RW__JSON__HPP
#define RW__JSON__HPP

#ifdef _MSVC_LANG
    #if (_MSVC_LANG < 202002L)
        #error rw::json is compatible with C++20 and higher
    #endif
#else
    #if (__cplusplus < 202002L)
        #error rw::json is compatible with C++20 and higher
    #endif
#endif

#include <iostream>       // For basic_i|o|iostram  | used by: json::reader, json::writer, json::serializer, json::deserializer
#include <vector>         // For vector             | used by: json::array
#include <unordered_map>  // For unordered_map      | used by: json::object
#include <variant>        // For variant            | used by: json::variant
#include <iomanip>        // For quoted             | used by: json::reader, json::writer, json::serializer, json::deserializer
#include <optional>       // For optional           | used by: json::serialize, json::deserialize
#include <sstream>        // For i|o|stringstream   | used by: json::reader, json::writer, json::serializer, json::deserializer
#include <memory>         // For smart pointers     | used by: json::variant
#include <array>          // For array              | used by: json::array

#ifndef RW_NAMESPACE
    #define RW_NAMESPACE                rw
    #define RW_NAMESPACE_BEGIN          namespace RW_NAMESPACE {
    #define RW_NAMESPACE_END            }
#endif

#define RW_JSON_NAMESPACE           json
#define RW_JSON_NAMESPACE_BEGIN     namespace RW_JSON_NAMESPACE {
#define RW_JSON_NAMESPACE_END       }

RW_NAMESPACE_BEGIN
RW_JSON_NAMESPACE_BEGIN

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_null;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_string;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_number;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_array;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_object;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_boolean;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_variant;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_writer;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_reader;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_serializer;

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_deserializer;

namespace detail {
    template<typename T, typename StdVariant>
    struct variant_alternative_index;

    template<typename T, typename StdVariant>
    struct user_input;

    template<typename T, typename StdVariant>
    struct user_output;

    template<typename T>
    concept has_begin_end = requires(const T& v) {
        { v.begin() } -> std::same_as<typename T::const_iterator>;
        { v.end()   } -> std::same_as<typename T::const_iterator>;
    } && requires(T& v) {
        { v.begin() } -> std::same_as<typename T::iterator>;
        { v.end()   } -> std::same_as<typename T::iterator>;
    };

    template<typename T>
    concept has_emplace_back = requires(T& v) {
        { v.emplace_back() } -> std::same_as<typename T::value_type&>;
    };

    template<typename T>
    concept has_insert_or_assign = requires(T& v) {
        { v.insert_or_assign(typename T::key_type(), typename T::mapped_type()) } -> std::same_as<std::pair<typename T::iterator, bool>>;
    };
}

template<typename Allocator, typename T>
struct rebind_alloc;

template<template<typename> typename Allocator, typename S, typename T>
struct rebind_alloc<Allocator<S>, T> {
    using type = Allocator<T>;
};

template<typename Allocator, typename T>
using rebind_alloc_t = typename rebind_alloc<Allocator, T>::type;

template<typename T>
concept is_input = std::is_default_constructible_v<T> && std::move_constructible<T> && std::is_move_assignable_v<T>;

template<typename T>
concept is_output = std::is_default_constructible_v<T> && std::move_constructible<T> && std::is_move_assignable_v<T>;

template<typename In, typename Out>
concept is_user_input = is_input<In> && requires(const In& in, Out& out) {
    { out << in } -> std::same_as<Out&>;
};

template<typename Out, typename In>
concept is_user_output = is_output<Out> && requires(const In& in, Out& out){
    { in >> out } -> std::same_as<const In&>;
};

template<typename T, typename JNull>
concept is_default_null_input = std::same_as<T, typename JNull::aliased_type>;

template<typename T, typename JNull>
concept is_default_null_output = std::same_as<T, typename JNull::aliased_type>;

template<typename T, typename JString>
concept is_default_string_input = is_input<T> && std::constructible_from<typename JString::aliased_type, T> && !std::same_as<T, std::nullptr_t>;

template<typename T, typename JString>
concept is_default_string_output = is_output<T> && std::constructible_from<T, typename JString::aliased_type>;

template<typename T, typename JNumber>
concept is_default_number_input = std::is_arithmetic_v<T> && !std::same_as<T, bool>;

template<typename T, typename JNumber>
concept is_default_number_output = std::is_arithmetic_v<T> && !std::same_as<T, bool>;

template<typename T, typename JArray>
concept is_default_array_input = is_input<T> && detail::has_begin_end<T> && is_user_input<typename T::value_type, typename JArray::value_type>;

template<typename T, typename JArray>
concept is_default_array_output = is_output<T> && detail::has_begin_end<T> && detail::has_emplace_back<T> && is_user_output<typename T::value_type, typename JArray::value_type>;

template<typename T, typename JObject>
concept is_default_object_input = is_input<T> && detail::has_begin_end<T> && std::constructible_from<typename JObject::key_type, typename T::key_type> && is_user_input<typename T::mapped_type, typename JObject::mapped_type>;

template<typename T, typename JObject>
concept is_default_object_output = is_output<T> && detail::has_begin_end<T> && detail::has_insert_or_assign<T> && std::constructible_from<typename T::key_type, typename JObject::key_type> && is_user_output<typename T::mapped_type, typename JObject::mapped_type>;

template<typename T, typename JBoolean>
concept is_default_boolean_input = std::same_as<T, typename JBoolean::aliased_type>;

template<typename T, typename JBoolean>
concept is_default_boolean_output = std::same_as<T, typename JBoolean::aliased_type>;

template<typename T, typename JVariant>
concept is_variant_alternative = detail::variant_alternative_index<T, typename JVariant::aliased_type>::value != std::variant_npos;

template<typename T, typename JVariant>
concept is_default_variant_input = is_variant_alternative<typename detail::user_input<T, typename JVariant::aliased_type>::type, JVariant>;

template<typename T, typename JVariant>
concept is_default_variant_output = is_variant_alternative<typename detail::user_output<T, typename JVariant::aliased_type>::type, JVariant>;

template<typename T, typename Char, typename Traits, typename Allocator>
concept is_writable = requires(const T& v, std::basic_ostream<Char, Traits>& os, basic_writer<Char, Traits, Allocator>& w) {
    { w(os, v) } -> std::same_as<std::basic_ostream<Char, Traits>&>;
};

template<typename T, typename Char, typename Traits, typename Allocator>
concept is_readable = requires(T& v, std::basic_istream<Char, Traits>& is, const basic_reader<Char, Traits, Allocator>& r) {
    { r(is, v) } -> std::same_as<std::basic_istream<Char, Traits>&>;
};

template<typename T, typename Char, typename Traits, typename Allocator>
concept is_serializable = (is_user_input<T, basic_variant<Char, Traits, Allocator>> || is_writable<T, Char, Traits, Allocator>) && !std::is_pointer_v<std::decay_t<T>>;

template<typename T, typename Char, typename Traits, typename Allocator>
concept is_deserializable = (is_user_output<T, basic_variant<Char, Traits, Allocator>> || is_readable<T, Char, Traits, Allocator>) && !std::is_pointer_v<std::decay_t<T>>;

namespace detail {
    template<typename T, typename StdVariant>
    struct variant_alternative_index {
        template<std::size_t N = 0>
        static constexpr std::size_t impl(void) noexcept {
            if constexpr (std::same_as<T, std::variant_alternative_t<N, StdVariant>>) {
                return N;
            }
            if constexpr (N < (std::variant_size_v<StdVariant> - 1)) {
                return impl<N + 1>();
            }
            return std::variant_npos;
        }

        static constexpr std::size_t value = impl();
    };

    template<typename T, typename StdVariant>
    inline constexpr std::size_t variant_alternative_index_v = variant_alternative_index<T, StdVariant>::value;

    template<typename T, typename StdVariant>
    struct user_input {
        template<std::size_t N = 0>
        static constexpr auto impl(void) noexcept {
            if constexpr (is_user_input<T, std::variant_alternative_t<N, StdVariant>>) {
                return std::variant_alternative_t<N, StdVariant>{};
            }
            else {
                if constexpr (N < (std::variant_size_v<StdVariant> - 1)) {
                    return impl<N + 1>();
                }
            }
        }

        using type = decltype(impl());
    };

    template<typename T, typename StdVariant>
    using user_input_t = typename user_input<T, StdVariant>::type;

    template<typename T, typename StdVariant>
    struct user_output {
        template<std::size_t N = 0>
        static constexpr auto impl(void) noexcept {
            if constexpr (is_user_output<T, std::variant_alternative_t<N, StdVariant>>) {
                return std::variant_alternative_t<N, StdVariant>{};
            }
            else {
                if constexpr (N < (std::variant_size_v<StdVariant> - 1)) {
                    return impl<N + 1>();
                }
            }
        }

        using type = decltype(impl());
    };

    template<typename T, typename StdVariant>
    using user_output_t = typename user_output<T, StdVariant>::type;
}

template<typename Char, typename Traits, typename Allocator>
class basic_null {
public:
    using aliased_type = std::nullptr_t;

    template<typename T> requires is_default_null_input<T, basic_null<Char, Traits, Allocator>>
    basic_null& operator<<(const T& value) {
        _Value = aliased_type(value);
        return *this;
    }

    template<typename T> requires is_default_null_output<T, basic_null<Char, Traits, Allocator>>
    const basic_null& operator>>(T& value) const {
        value = T(_Value);
        return *this;
    }

    aliased_type& get(void) noexcept {
        return _Value;
    }

    const aliased_type& get(void) const noexcept {
        return _Value;
    }

private:
    aliased_type _Value{};
};

template<typename Char, typename Traits, typename Allocator>
class basic_string {
public:
    using aliased_type = std::basic_string<Char, Traits, Allocator>;

    template<typename T> requires is_default_string_input<T, basic_string<Char, Traits, Allocator>>
    basic_string& operator<<(const T& value) {
        _Value = aliased_type(value);
        return *this;
    }

    template<typename T> requires is_default_string_output<T, basic_string<Char, Traits, Allocator>>
    const basic_string& operator>>(T& value) const {
        value = T(_Value);
        return *this;
    }

    aliased_type& get(void) noexcept {
        return _Value;
    }

    const aliased_type& get(void) const noexcept {
        return _Value;
    }

private:
    aliased_type _Value{};
};

template<typename Char, typename Traits, typename Allocator>
class basic_number {
public:
    using aliased_type = std::double_t;

    template<typename T> requires is_default_number_input<T, basic_number<Char, Traits, Allocator>>
    basic_number& operator<<(const T& value) {
        _Value = aliased_type(value);
        return *this;
    }

    template<typename T> requires is_default_number_output<T, basic_number<Char, Traits, Allocator>>
    const basic_number& operator>>(T& value) const {
        value = T(_Value);
        return *this;
    }

    aliased_type& get(void) noexcept {
        return _Value;
    }

    const aliased_type& get(void) const noexcept {
        return _Value;
    }

private:
    aliased_type _Value{};
};

template<typename Char, typename Traits, typename Allocator>
class basic_array {
public:
    using value_type      = basic_variant<Char, Traits, Allocator>;
    using allocator_type  = typename rebind_alloc_t<Allocator, value_type>;
    using aliased_type    = std::vector<value_type, allocator_type>;
    using size_type       = typename aliased_type::size_type;
    using difference_type = typename aliased_type::difference_type;
    using iterator        = typename aliased_type::iterator;
    using const_iterator  = typename aliased_type::const_iterator;

    template<typename T> requires is_default_array_input<T, basic_array<Char, Traits, Allocator>>
    basic_array& operator<<(const T& value) {
        aliased_type _Temp{};
        for(const auto& v : value) {
            _Temp.emplace_back() << v;
        }
        _Value = std::move(_Temp);
        return *this;
    }

    template<typename T> requires is_default_array_output<T, basic_array<Char, Traits, Allocator>>
    const basic_array& operator>>(T& value) const {
        T _Temp{};
        for (const auto& v : _Value) {
            v >> _Temp.emplace_back();
        }
        value = std::move(_Temp);
        return *this;
    }

    template<typename T, size_type N> requires is_user_input<T, value_type>
    basic_array& operator<<(const std::array<T, N>& value) {
        aliased_type _Temp{};
        for (const auto& v : value) {
            _Temp.emplace_back() << v;
        }
        _Value = std::move(_Temp);
        return *this;
    }

    template<typename T, size_type N> requires is_user_output<T, value_type>
    const basic_array& operator<<(const std::array<T, N>& value) const {
        for (size_type n = 0; n < N; ++n) {
            _Value[n] >> value[n];
        }
        return *this;
    }

    iterator begin(void) noexcept {
        return _Value.begin();
    }

    iterator end(void) noexcept {
        return _Value.end();
    }

    const_iterator begin(void) const noexcept {
        return _Value.begin();
    }

    const_iterator end(void) const noexcept {
        return _Value.end();
    }

    const_iterator cbegin(void) const noexcept {
        return _Value.cbegin();
    }

    const_iterator cend(void) const noexcept {
        return _Value.cend();
    }

    size_type size(void) const noexcept {
        return _Value.size();
    }

    bool contains(size_type idx) const noexcept {
        return idx < size();
    }

    value_type& operator[](size_type idx) {
        difference_type difference = idx - size() + 1;
        for (difference_type i = 0; i < difference && difference > 0; ++i) {
            _Value.emplace_back();
        }
        return _Value[idx];
    }

    const value_type& operator[](size_type idx) const {
        return _Value[idx];
    }

    aliased_type& get(void) noexcept {
        return _Value;
    }

    const aliased_type& get(void) const noexcept {
        return _Value;
    }

private:
    aliased_type _Value{};
};

template<typename Char, typename Traits, typename Allocator>
class basic_object {
public:
    using key_type        = std::basic_string<Char, Traits, Allocator>;
    using mapped_type     = basic_variant<Char, Traits, Allocator>;
    using hasher          = std::hash<key_type>;
    using key_equal       = std::equal_to<key_type>;
    using allocator_type  = typename rebind_alloc_t<Allocator, std::pair<const key_type, mapped_type>>;
    using aliased_type    = std::unordered_map<key_type, mapped_type, hasher, key_equal, allocator_type>;
    using size_type       = typename aliased_type::size_type;
    using difference_type = typename aliased_type::difference_type;
    using iterator        = typename aliased_type::iterator;
    using const_iterator  = typename aliased_type::const_iterator;

    template<typename T> requires is_default_object_input<T, basic_object<Char, Traits, Allocator>>
    basic_object& operator<<(const T& value) {
        aliased_type _Temp{};
        for (const auto& [k, v] : value) {
            _Temp[key_type(k)] << v;
        }
        _Value = std::move(_Temp);
        return *this;
    }

    template<typename T> requires is_default_object_output<T, basic_object<Char, Traits, Allocator>>
    const basic_object& operator>>(T& value) const {
        T _Temp{};
        for (const auto& [k, v] : _Value) {
            typename T::value_type _v{};
            _Temp.insert_or_assign(k, std::move(v >> _v));
        }
        value = std::move(_Temp);
        return *this;
    }

    iterator begin(void) noexcept {
        return _Value.begin();
    }

    iterator end(void) noexcept {
        return _Value.end();
    }

    const_iterator begin(void) const noexcept {
        return _Value.begin();
    }

    const_iterator end(void) const noexcept {
        return _Value.end();
    }

    const_iterator cbegin(void) const noexcept {
        return _Value.cbegin();
    }

    const_iterator cend(void) const noexcept {
        return _Value.cend();
    }

    size_type size(void) const noexcept {
        return _Value.size();
    }

    bool contains(const key_type& key) const noexcept {
        return _Value.contains(key);
    }

    mapped_type& operator[](const key_type& key) {
        return _Value[key];
    }

    mapped_type& operator[](key_type&& key) {
        return _Value[std::move(key)];
    }

    const mapped_type& operator[](const key_type& key) const {
        return _Value.at(key);
    }

    aliased_type& get(void) noexcept {
        return _Value;
    }

    const aliased_type& get(void) const noexcept {
        return _Value;
    }

private:
    aliased_type _Value{};
};

template<typename Char, typename Traits, typename Allocator>
class basic_boolean {
public:
    using aliased_type = bool;

    template<typename T> requires is_default_boolean_input<T, basic_boolean<Char, Traits, Allocator>>
    basic_boolean& operator<<(const T& value) {
        _Value = aliased_type(value);
        return *this;
    }

    template<typename T> requires is_default_boolean_output<T, basic_boolean<Char, Traits, Allocator>>
    const basic_boolean& operator>>(T& value) const {
        value = T(_Value);
        return *this;
    }

    aliased_type& get(void) noexcept {
        return _Value;
    }

    const aliased_type& get(void) const noexcept {
        return _Value;
    }

private:
    aliased_type _Value{};
};

template<typename Char, typename Traits, typename Allocator>
class basic_variant {
public:
     using null_type    = basic_null<Char, Traits, Allocator>;
     using string_type  = basic_string<Char, Traits, Allocator>;
     using number_type  = basic_number<Char, Traits, Allocator>;
     using array_type   = basic_array<Char, Traits, Allocator>;
     using object_type  = basic_object<Char, Traits, Allocator>;
     using boolean_type = basic_boolean<Char, Traits, Allocator>;
     using aliased_type = std::variant<null_type, string_type, number_type, array_type, object_type, boolean_type>;

    template<typename T> requires is_variant_alternative<T, basic_variant<Char, Traits, Allocator>>
    bool is(void) const noexcept {
        return _Value.index() == detail::variant_alternative_index_v<T, aliased_type>;
    }

    template<typename T> requires is_variant_alternative<T, basic_variant<Char, Traits, Allocator>>
    T& to(void) {
        if (!this->is<T>()) {
            _Value = T{};
        }
        return std::get<T>(_Value);
    }

    template<typename T> requires is_variant_alternative<T, basic_variant<Char, Traits, Allocator>>
    T& to(T&& value) {
        _Value = std::move(value);
        return std::get<T>(_Value);
    }

    template<typename T> requires is_variant_alternative<T, basic_variant<Char, Traits, Allocator>>
    T& as(void) {
        return std::get<T>(_Value);
    }

    template<typename T> requires is_variant_alternative<T, basic_variant<Char, Traits, Allocator>>
    const T& as(void) const {
        return std::get<T>(_Value);
    }

    template<typename T> requires is_variant_alternative<T, basic_variant<Char, Traits, Allocator>>
    T& as(T& def) noexcept {
        if (!this->is<T>()) {
            return def;
        }
        return std::get<T>(_Value);
    }

    template<typename T> requires is_variant_alternative<T, basic_variant<Char, Traits, Allocator>>
    const T& as(const T& def) const noexcept {
        if (!this->is<T>()) {
            return def;
        }
        return std::get<T>(_Value);
    }

    template<typename T> requires is_default_variant_input<T, basic_variant<Char, Traits, Allocator>>
    basic_variant& operator<<(const T& value) {
        this->to<detail::user_input_t<T, aliased_type>>() << value;
        return *this;
    }

    template<typename T, typename Deleter> requires is_default_variant_input<T, basic_variant<Char, Traits, Allocator>>
    basic_variant& operator<<(const std::unique_ptr<T, Deleter>& ptr) {
        if (!ptr) {
            this->to<null_type>();
            return *this;
        }
        return (*this) << (*ptr);
    }

    template<typename T> requires is_default_variant_input<T, basic_variant<Char, Traits, Allocator>>
    basic_variant& operator<<(const std::shared_ptr<T>& ptr) {
        if (!ptr) {
            this->to<null_type>();
            return *this;
        }
        return (*this) << (*ptr);
    }

    template<typename T> requires is_default_variant_output<T, basic_variant<Char, Traits, Allocator>>
    const basic_variant& operator>>(T& value) const {
        this->as<detail::user_output_t<T, aliased_type>>() >> value;
        return *this;
    }

    template<typename T, typename Deleter> requires is_default_variant_output<T, basic_variant<Char, Traits, Allocator>>
    const basic_variant& operator>>(std::unique_ptr<T, Deleter>& ptr) const {
        if (this->is<null_type>()) {
            ptr.reset();
            return *this;
        }
        if (!ptr) {
            ptr.reset(new T());
        }
        return (*this) >> (*ptr);
    }

    template<typename T> requires is_default_variant_output<T, basic_variant<Char, Traits, Allocator>>
    const basic_variant& operator>>(std::shared_ptr<T>& ptr) const {
        if (this->is<null_type>()) {
            ptr.reset();
            return *this;
        }
        if (!ptr) {
            ptr.reset(new T());
        }
        return (*this) >> (*ptr);
    }

    aliased_type& get(void) noexcept {
        return _Value;
    }

    const aliased_type& get(void) const noexcept {
        return _Value;
    }

private:
    aliased_type _Value{ null_type{} };
};

template<typename Char, typename Traits, typename Allocator>
class basic_writer {
public:
    basic_writer(bool with_indentation = false, int indentation = 0) noexcept
        : _WithIndentation(with_indentation)
        , _Indentation(indentation)
    {
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& operator()(std::basic_ostream<Char, DontCare...>& os, const typename basic_null<Char, Traits, Allocator>::aliased_type& null) {
        if constexpr (std::same_as<Char, char>) {
            os << "null";
        }
        if constexpr (std::same_as<Char, wchar_t>) {
            os << L"null";
        }
        if constexpr (std::same_as<Char, char8_t>) {
            os << u8"null";
        }
        if constexpr (std::same_as<Char, char16_t>) {
            os << u"null";
        }
        if constexpr (std::same_as<Char, char32_t>) {
            os << U"null";
        }
        return this->validate(os);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& operator()(std::basic_ostream<Char, DontCare...>& os, const typename basic_string<Char, Traits, Allocator>::aliased_type& str) {
        os << std::quoted(str);
        return this->validate(os);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& operator()(std::basic_ostream<Char, DontCare...>& os, const typename basic_number<Char, Traits, Allocator>::aliased_type& num) {
        os << num;
        return this->validate(os);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& operator()(std::basic_ostream<Char, DontCare...>& os, const typename basic_array<Char, Traits, Allocator>::aliased_type& arr) {
        this->begin(os, Char('['));
        if (arr.size()) {
            auto it = arr.begin();
            this->element(os, it->get());
            ++it;
            while (it != arr.end()) {
                this->element(os, it->get(), Char(','));
                ++it;
            }
        }
        return this->end(os, Char(']'));
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& operator()(std::basic_ostream<Char, DontCare...>& os, const typename basic_object<Char, Traits, Allocator>::aliased_type& obj) {
        this->begin(os, Char('{'));
        if (obj.size()) {
            auto it = obj.begin();
            this->element(os, it->first, it->second.get());
            ++it;
            while (it != obj.end()) {
                this->element(os, it->first, it->second.get(), Char(','));
                ++it;
            }
        }
        return this->end(os, Char('}'));
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& operator()(std::basic_ostream<Char, DontCare...>& os, const typename basic_boolean<Char, Traits, Allocator>::aliased_type& b) {
        return this->validate(os << std::boolalpha << b);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& operator()(std::basic_ostream<Char, DontCare...>& os, const typename basic_variant<Char, Traits, Allocator>::aliased_type& var) {
        return std::visit([&](const auto& value) -> std::basic_ostream<Char, DontCare...>& {
            return this->operator()(os, value.get());
        }, var);
    }

private:
    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& validate(std::basic_ostream<Char, DontCare...>& os) {
        if (!os.good()) {
            throw std::exception("writing to stream failed");
        }
        return os;
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& begin(std::basic_ostream<Char, DontCare...>& os, const Char& c) {
        this->validate(os << c);
        ++_Indentation;
        return os;
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& end(std::basic_ostream<Char, DontCare...>& os, const Char& c) {
        this->linebreak(os);
        --_Indentation;
        this->indent(os);
        return this->validate(os << c);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& element(std::basic_ostream<Char, DontCare...>& os, const typename basic_variant<Char, Traits, Allocator>::aliased_type& var) {
        this->linebreak(os);
        this->indent(os);
        return this->operator()(os, var);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& element(std::basic_ostream<Char, DontCare...>& os, const typename basic_variant<Char, Traits, Allocator>::aliased_type& var, const Char& prefix) {
        this->validate(os << prefix);
        this->linebreak(os);
        this->indent(os);
        return this->operator()(os, var);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& element(std::basic_ostream<Char, DontCare...>& os, const typename basic_object<Char, Traits, Allocator>::key_type& key, const typename basic_variant<Char, Traits, Allocator>::aliased_type& var) {
        this->linebreak(os);
        this->indent(os);
        this->validate(os << std::quoted(key) << Char(':'));
        this->space(os);
        return this->operator()(os, var);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& element(std::basic_ostream<Char, DontCare...>& os, const typename basic_object<Char, Traits, Allocator>::key_type& key, const typename basic_variant<Char, Traits, Allocator>::aliased_type& var, const Char& prefix) {
        this->validate(os << prefix);
        this->linebreak(os);
        this->indent(os);
        this->validate(os << std::quoted(key) << Char(':'));
        this->space(os);
        return this->operator()(os, var);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& linebreak(std::basic_ostream<Char, DontCare...>& os) {
        if (!_WithIndentation) {
            return os;
        }
        return this->validate(os << Char('\n'));
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& indent(std::basic_ostream<Char, DontCare...>& os) {
        if (!_WithIndentation) {
            return os;
        }
        for (int i = 0; i < _Indentation; ++i) {
            os << Char('\t');
        }
        return this->validate(os);
    }

    template<typename ... DontCare>
    std::basic_ostream<Char, DontCare...>& space(std::basic_ostream<Char, DontCare...>& os) {
        if (!_WithIndentation) {
            return os;
        }
        return this->validate(os << Char(' '));
    }

    bool _WithIndentation{ false };
    int  _Indentation{ 0 };
};

template<typename Char, typename Traits, typename Allocator>
class basic_reader {
public:
    template<typename ... DontCare>
    std::basic_istream<Char, DontCare...>& operator()(std::basic_istream<Char, DontCare...>& is, typename basic_null<Char, Traits, Allocator>::aliased_type& null) const {
        Char str[5]{};
        this->validate(is >> str);

        bool _Equals = false;

        if constexpr (std::same_as<Char, char>) {
            _Equals = std::memcmp(str, "null", 5 * sizeof(Char)) == 0;
        }
        if constexpr (std::same_as<Char, wchar_t>) {
            _Equals = std::memcmp(str, L"null", 5 * sizeof(Char)) == 0;
        }
        if constexpr (std::same_as<Char, char8_t>) {
            _Equals = std::memcmp(str, u8"null", 5 * sizeof(Char)) == 0;
        }
        if constexpr (std::same_as<Char, char16_t>) {
            _Equals = std::memcmp(str, u"null", 5 * sizeof(Char)) == 0;
        }
        if constexpr (std::same_as<Char, char32_t>) {
            _Equals = std::memcmp(str, U"null", 5 * sizeof(Char)) == 0;
        }

        if (!_Equals) {
            throw std::exception("unexpected literal encountered, expected <null>");
        }

        return is;
    }

    template<typename ... DontCare>
    std::basic_istream<Char, DontCare...>& operator()(std::basic_istream<Char, DontCare...>& is, typename basic_string<Char, Traits, Allocator>::aliased_type& str) const {
        typename basic_string<Char, Traits, Allocator>::aliased_type _Temp{};
        this->validate(is >> std::quoted(_Temp));
        str = std::move(_Temp);
        return is;
    }

    template<typename ... DontCare>
    std::basic_istream<Char, DontCare...>& operator()(std::basic_istream<Char, DontCare...>& is, typename basic_number<Char, Traits, Allocator>::aliased_type& num) const {
        typename basic_number<Char, Traits, Allocator>::aliased_type _Temp{};
        this->validate(is >> _Temp);
        num = std::move(_Temp);
        return is;
    }

    template<typename ... DontCare>
    std::basic_istream<Char, DontCare...>& operator()(std::basic_istream<Char, DontCare...>& is, typename basic_array<Char, Traits, Allocator>::aliased_type& arr) const {
        typename basic_array<Char, Traits, Allocator>::aliased_type _Temp{};
        Char c{};

        this->validate(is >> c);
        if (c != Char('[')) {
            throw std::exception("unexpected token encountered, expected <[>");
        }

        do {
            this->operator()(is, _Temp.emplace_back().get());
            this->validate(is >> c);
        } while (c == Char(','));

        if (c != Char(']')) {
            throw std::exception("unexpected token encountered, expected <,> or <]>");
        }

        arr = std::move(_Temp);

        return is;
    }

    template<typename ... DontCare>
    std::basic_istream<Char, DontCare...>& operator()(std::basic_istream<Char, DontCare...>& is, typename basic_object<Char, Traits, Allocator>::aliased_type& obj) const {
        typename basic_object<Char, Traits, Allocator>::aliased_type _Temp{};
        Char c{};

        this->validate(is >> c);
        if (c != Char('{')) {
            throw std::exception("unexpected token encountered, expected <{>");
        }

        do {
            std::basic_string<Char, Traits, Allocator> _Key{};
            this->validate(is >> std::quoted(_Key));

            this->validate(is >> c);
            if (c != Char(':')) {
                throw std::exception("unexpected token encountered, expected <:>");
            }

            this->operator()(is, _Temp[std::move(_Key)].get());

            this->validate(is >> c);
        } while (c == Char(','));

        if (c != Char('}')) {
            throw std::exception("unexpected token encountered, expected <,> or <}>");
        }

        obj = std::move(_Temp);

        return is;
    }

    template<typename ... DontCare>
    std::basic_istream<Char, DontCare...>& operator()(std::basic_istream<Char, DontCare...>& is, typename basic_boolean<Char, Traits, Allocator>::aliased_type& b) const {
        typename basic_boolean<Char, Traits, Allocator>::aliased_type _Temp{};
        this->validate(is >> std::boolalpha >> _Temp);
        b = std::move(_Temp);
        return is;
    }

    template<typename ... DontCare>
    std::basic_istream<Char, DontCare...>& operator()(std::basic_istream<Char, DontCare...>& is, typename basic_variant<Char, Traits, Allocator>::aliased_type& var) const {
        this->validate(is >> std::ws);
        switch (is.peek()) {
        case Char('n'): {
            basic_null<Char, Traits, Allocator> null{};
            operator()(is, null.get());
            var = std::move(null);
        } break;

        case Char('"'): {
            basic_string<Char, Traits, Allocator> str{};
            operator()(is, str.get());
            var = std::move(str);
        } break;

        case Char('0'):
        case Char('1'):
        case Char('2'):
        case Char('3'):
        case Char('4'):
        case Char('5'):
        case Char('6'):
        case Char('7'):
        case Char('8'):
        case Char('9'): {
            basic_number<Char, Traits, Allocator> num{};
            operator()(is, num.get());
            var = std::move(num);
        } break;

        case Char('['): {
            basic_array<Char, Traits, Allocator> arr{};
            operator()(is, arr.get());
            var = std::move(arr);
        } break;

        case Char('{'): {
            basic_object<Char, Traits, Allocator> obj{};
            operator()(is, obj.get());
            var = std::move(obj);
        } break;

        case Char('t'):
        case Char('f'): {
            basic_boolean<Char, Traits, Allocator> b{};
            operator()(is, b.get());
            var = std::move(b);
        } break;

        default:
            throw std::exception("unexpected character encountered, expected <n>, <\">, <0>, <1>, <2>, <3>, <4>, <5>, <6>, <7>, <8>, <9>, <[>, <{>, <t> or <f>");
        }
        return is;
    }

private:
    template<typename ... DontCare>
    std::basic_istream<Char, DontCare...>& validate(std::basic_istream<Char, DontCare...>& is) const {
        if (!is.good() && !is.eof()) {
            throw std::exception("reading from stream failed");
        }
        return is;
    }
};

template<typename Char, typename Traits, typename Allocator>
class basic_serializer {
public:
    basic_serializer(bool use_indentation = false, int indentation = 0) noexcept
        : _Writer(use_indentation, indentation)
        , _Error{}
    {
    }

    template<typename T, typename ... DontCare> requires is_serializable<T, Char, Traits, Allocator>
    basic_serializer& operator()(std::basic_ostream<Char, DontCare...>& os, const T& value) noexcept {
        try {
            if constexpr (is_writable<T, Char, Traits, Allocator>) {
                _Writer(os, value.get());
            }
            else {
                basic_variant<Char, Traits, Allocator> _Var{};
                _Writer(os, (_Var << value).get());
            }
            _Error = std::optional<std::exception>{};
        }
        catch (std::exception e) {
            _Error = std::move(e);
        }
        return *this;
    }

    template<typename T, typename ... DontCare> requires is_serializable<T, Char, Traits, Allocator>
    basic_serializer& operator()(std::basic_string<Char, DontCare...>& str, const T& value) noexcept {
        try {
            std::basic_ostringstream<Char, DontCare...> os{};
            if constexpr (is_writable<T, Char, Traits, Allocator>) {
                _Writer(os, value.get());
            }
            else {
                basic_variant<Char, Traits, Allocator> _Var{};
                _Writer(os, (_Var << value).get());
            }
            str = os.str();
            _Error = std::optional<std::exception>{};
        }
        catch (std::exception e) {
            _Error = std::move(e);
        }
        return *this;
    }

    operator bool() const noexcept {
        return !_Error.has_value();
    }

    const std::exception& error(void) const {
        return _Error.value();
    }

private:
    basic_writer<Char, Traits, Allocator> _Writer{};
    std::optional<std::exception>         _Error{};
};

template<typename Char, typename Traits, typename Allocator>
class basic_deserializer {
public:
    template<typename T, typename ... DontCare> requires is_deserializable<T, Char, Traits, Allocator>
    basic_deserializer& operator()(std::basic_istream<Char, DontCare...>& is, T& value) noexcept {
        try {
            if constexpr (is_readable<T, Char, Traits, Allocator>) {
                _Reader(is, value.get());
            }
            else {
                basic_variant<Char, Traits, Allocator> _Var{};
                _Reader(is, _Var.get());
                _Var >> value;
            }
            _Error = std::optional<std::exception>{};
        }
        catch (std::exception e) {
            _Error = std::move(e);
        }
        return *this;
    }

    template<typename T, typename ... DontCare> requires is_deserializable<T, Char, Traits, Allocator>
    basic_deserializer& operator()(const std::basic_string<Char, DontCare...>& str, T& value) noexcept {
        try {
            std::basic_istringstream<Char, DontCare...> is{ str };
            if constexpr (is_readable<T, Char, Traits, Allocator>) {
                _Reader(is, value.get());
            }
            else {
                basic_variant<Char, Traits, Allocator> _Var{};
                _Reader(is, _Var.get());
                _Var >> value;
            }
            _Error = std::optional<std::exception>{};
        }
        catch (std::exception e) {
            _Error = std::move(e);
        }
        return *this;
    }

    operator bool() const noexcept {
        return !_Error.has_value();
    }

    const std::exception& error(void) const {
        return _Error.value();
    }

private:
    basic_reader<Char, Traits, Allocator> _Reader{};
    std::optional<std::exception>         _Error{};
};

template<typename T, typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>> requires is_serializable<T, Char, Traits, Allocator>
inline std::optional<std::exception> serialize(std::basic_ostream<Char, Traits>& os, const T& value, bool use_indentation = false, int indentation = 0) noexcept {
    basic_serializer<Char, Traits, Allocator> js{ use_indentation, indentation };
    if(!js(os, value)) {
        return js.error();
    }
    return std::optional<std::exception>{};
}

template<typename T, typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>> requires is_serializable<T, Char, Traits, Allocator>
inline std::optional<std::exception> serialize(std::basic_string<Char, Traits, Allocator>& str, const T& value, bool use_indentation = false, int indentation = 0) noexcept {
    basic_serializer<Char, Traits, Allocator> js{ use_indentation, indentation };
    if(!js(str, value)) {
        return js.error();
    }
    return std::optional<std::exception>{};
}

template<typename T, typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>> requires is_deserializable<T, Char, Traits, Allocator>
inline std::optional<std::exception> deserialize(std::basic_istream<Char, Traits>& is, T& value) noexcept {
    basic_deserializer<Char, Traits, Allocator> jds{};
    if (!jds(is, value)) {
        return jds.error();
    }
    return std::optional<std::exception>{};
}

template<typename T, typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>> requires is_deserializable<T, Char, Traits, Allocator>
inline std::optional<std::exception> deserialize(const std::basic_string<Char, Traits, Allocator>& str, T& value) noexcept {
    basic_deserializer<Char, Traits, Allocator> jds{};
    if (!jds(str, value)) {
        return jds.error();
    }
    return std::optional<std::exception>{};
}

using variant      = basic_variant<char>;
using null         = basic_null<char>;
using string       = basic_string<char>;
using number       = basic_number<char>;
using array        = basic_array<char>;
using object       = basic_object<char>;
using boolean      = basic_boolean<char>;
using reader       = basic_reader<char>;
using writer       = basic_writer<char>;
using serializer   = basic_serializer<char>;
using deserializer = basic_deserializer<char>;

using wvariant      = basic_variant<wchar_t>;
using wnull         = basic_null<wchar_t>;
using wstring       = basic_string<wchar_t>;
using wnumber       = basic_number<wchar_t>;
using warray        = basic_array<wchar_t>;
using wobject       = basic_object<wchar_t>;
using wboolean      = basic_boolean<wchar_t>;
using wreader       = basic_reader<wchar_t>;
using wwriter       = basic_writer<wchar_t>;
using wserializer   = basic_serializer<wchar_t>;
using wdeserializer = basic_deserializer<wchar_t>;

using u8variant      = basic_variant<char8_t>;
using u8null         = basic_null<char8_t>;
using u8string       = basic_string<char8_t>;
using u8number       = basic_number<char8_t>;
using u8array        = basic_array<char8_t>;
using u8object       = basic_object<char8_t>;
using u8boolean      = basic_boolean<char8_t>;
using u8reader       = basic_reader<char8_t>;
using u8writer       = basic_writer<char8_t>;
using u8serializer   = basic_serializer<char8_t>;
using u8deserializer = basic_deserializer<char8_t>;

using u16variant      = basic_variant<char16_t>;
using u16null         = basic_null<char16_t>;
using u16string       = basic_string<char16_t>;
using u16number       = basic_number<char16_t>;
using u16array        = basic_array<char16_t>;
using u16object       = basic_object<char16_t>;
using u16boolean      = basic_boolean<char16_t>;
using u16reader       = basic_reader<char16_t>;
using u16writer       = basic_writer<char16_t>;
using u16serializer   = basic_serializer<char16_t>;
using u16deserializer = basic_deserializer<char16_t>;

using u32variant      = basic_variant<char32_t>;
using u32null         = basic_null<char32_t>;
using u32string       = basic_string<char32_t>;
using u32number       = basic_number<char32_t>;
using u32array        = basic_array<char32_t>;
using u32object       = basic_object<char32_t>;
using u32boolean      = basic_boolean<char32_t>;
using u32reader       = basic_reader<char32_t>;
using u32writer       = basic_writer<char32_t>;
using u32serializer   = basic_serializer<char32_t>;
using u32deserializer = basic_deserializer<char32_t>;

RW_JSON_NAMESPACE_END
RW_NAMESPACE_END

#endif
