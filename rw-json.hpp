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

#include <iostream>       // For basic_i|o|stream   | used by: json::reader, json::writer, json::serializer, json::deserializer
#include <vector>         // For vector             | used by: json::array
#include <unordered_map>  // For unordered_map      | used by: json::object
#include <variant>        // For variant            | used by: json::value
#include <iomanip>        // For quoted             | used by: json::reader, json::writer, json::serializer, json::deserializer
#include <optional>       // For optional           | used by: json::serialize, json::deserialize
#include <sstream>        // For i|o|stringstream   | used by: json::reader, json::writer, json::serializer, json::deserializer
#include <memory>         // For smart pointers     | used by: extensions
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

//
// Implementation details, or ugly traits and stuff...
//

namespace detail {
    template<typename Container>
    concept is_container = requires(const Container& c) {
        { c.begin() } -> std::same_as<typename Container::const_iterator>;
        { c.end()   } -> std::same_as<typename Container::const_iterator>;
    } && requires(Container& c) {
        { c.begin() } -> std::same_as<typename Container::iterator>;
        { c.end()   } -> std::same_as<typename Container::iterator>;
    } && requires { typename Container::value_type; };

    template<typename T>
    concept is_pair_container = is_container<T> && std::same_as<typename T::value_type, std::pair<typename T::value_type::first_type, typename T::value_type::second_type>>;

    template<typename T>
    concept is_single_container = is_container<T> && !is_pair_container<T>;

    template<typename T>
    concept is_quotable = requires { std::quoted(std::declval<T>()); };

    template<typename T, typename Allocator>
    using rebind_alloc_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  RAW API
// ---------
//  Provides json grammer for raw c++ types directly.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  TYPE_ID
// ---------
//  Do not change order, matches basic_value member order.
//

enum class type_id : std::size_t {
    invalid = ~0,
    null    =  0,
    string  =  1,
    number  =  2,
    array   =  3,
    object  =  4,
    boolean =  5
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// WRITER
//

template<typename Char, typename Traits = std::char_traits<Char>>
class basic_writer {
public:
    basic_writer(std::basic_ostream<Char, Traits>& os, bool indentation = false, int level = 0)
        : _Os(os)
        , _Indentation(indentation)
        , _Level(level)
    {
        this->indent();
    }

    basic_writer& operator<<(std::nullptr_t) {
        if constexpr (std::same_as<Char, char>) {
            _Os << "null";
        }
        if constexpr (std::same_as<Char, wchar_t>) {
            _Os << L"null";
        }
        if constexpr (std::same_as<Char, char8_t>) {
            _Os << u8"null";
        }
        if constexpr (std::same_as<Char, char16_t>) {
            _Os << u"null";
        }
        if constexpr (std::same_as<Char, char32_t>) {
            _Os << U"null";
        }
        return *this;
    }

    template<std::size_t N>
    basic_writer& operator<<(const Char(&str)[N]) {
        _Os << std::quoted(str);
        return *this;
    }

    template<typename ... Ts>
    basic_writer& operator<<(const std::basic_string<Char, Ts...>& str) {
        _Os << std::quoted(str);
        return *this;
    }

    template<typename ... Ts>
    basic_writer& operator<<(std::basic_string<Char, Ts...>&& str) {
        _Os << std::quoted(str);
        return *this;
    }

    template<typename ... Ts>
    basic_writer& operator<<(std::basic_string_view<Char, Ts...> str) {
        _Os << std::quoted(str);
        return *this;
    }

    template<typename T> requires (std::is_arithmetic_v<T> && !std::same_as<bool, T>)
        basic_writer& operator<<(const T& number) {
        _Os << number;
        return *this;
    }

    template<typename T> requires std::same_as<bool, T>
    basic_writer& operator<<(const T& object) {
        _Os << std::boolalpha << object;
        return *this;
    }

    template<typename Key, typename Value>
    basic_writer& operator<<(const std::pair<Key, Value>& pair) {
        if constexpr (detail::is_quotable<Key>) {
            _Os << std::quoted(pair.first);
        }
        else {
            _Os << Char('"');
            (*this) << pair.second;
            _Os << Char('"');
        }
        _Os << Char(':');
        this->space();
        (*this) << pair.second;
        return *this;
    }

    template<typename Container> requires detail::is_single_container<Container>
    basic_writer& operator<<(const Container& container) {
        this->begin(Char('['));
        auto it = container.begin();

        this->linebreak();
        this->indent();
        (*this) << (*it);
        ++it;

        do {
            _Os << Char(',');
            this->linebreak();
            this->indent();
            (*this) << (*it);
            ++it;
        } while (it != container.end());
        this->end(Char(']'));
        return *this;
    }

    template<typename Container> requires detail::is_pair_container<Container>
    basic_writer& operator<<(const Container& container) {
        this->begin(Char('{'));
        auto it = container.begin();

        this->linebreak();
        this->indent();
        (*this) << (*it);
        ++it;

        do {
            _Os << Char(',');
            this->linebreak();
            this->indent();
            (*this) << (*it);
            ++it;
        } while (it != container.end());
        this->end(Char('}'));
        return *this;
    }

    operator bool() const noexcept {
        return !(_Os.bad() || _Os.fail());
    }

protected:
    void begin(const Char& c) {
        _Os << c;
        ++_Level;
    }

    void end(const Char& c) {
        this->linebreak();
        --_Level;
        this->indent();
        _Os << c;
    }

    void linebreak(void) {
        if (_Indentation) {
            _Os << Char('\n');
        }
    }

    void indent(void) {
        for (int i = 0; i < _Level && _Indentation; ++i) {
            _Os << Char('\t');
        }
    }

    void space(void) {
        if (_Indentation) {
            _Os << Char(' ');
        }
    }

    std::basic_ostream<Char, Traits>& _Os;
    bool                              _Indentation{ false };
    int                               _Level{ 0 };
};

using writer    = basic_writer<char>;
using wwriter   = basic_writer<wchar_t>;
using u8writer  = basic_writer<char8_t>;
using u16writer = basic_writer<char16_t>;
using u32writer = basic_writer<char32_t>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// READER
//

template<typename Char, typename Traits = std::char_traits<Char>>
class basic_reader {
public:
    basic_reader(std::basic_istream<Char, Traits>& is)
        : _Is(is)
    {
    }

    type_id type(void) const noexcept {
        _Is >> std::ws;
        Char ch = _Is.peek();
        switch (ch) {
        case Char('n'): return type_id::null;
        case Char('"'): return type_id::string;
        case Char('0'): return type_id::number;
        case Char('1'): return type_id::number;
        case Char('2'): return type_id::number;
        case Char('3'): return type_id::number;
        case Char('4'): return type_id::number;
        case Char('5'): return type_id::number;
        case Char('6'): return type_id::number;
        case Char('7'): return type_id::number;
        case Char('8'): return type_id::number;
        case Char('9'): return type_id::number;
        case Char('['): return type_id::array;
        case Char('{'): return type_id::object;
        case Char('t'): return type_id::boolean;
        default:
            break;
        }
        return type_id::invalid;
    }

    basic_reader& operator>>(std::nullptr_t) {
        Char ch[5]{};
        _Is >> ch;

        bool _Equals = false;

        if constexpr (std::same_as<Char, char>) {
            _Equals = std::memcmp(ch, "null", 5 * sizeof(Char)) == 0;
        }
        if constexpr (std::same_as<Char, wchar_t>) {
            _Equals = std::memcmp(ch, L"null", 5 * sizeof(Char)) == 0;
        }
        if constexpr (std::same_as<Char, char8_t>) {
            _Equals = std::memcmp(ch, u8"null", 5 * sizeof(Char)) == 0;
        }
        if constexpr (std::same_as<Char, char16_t>) {
            _Equals = std::memcmp(ch, u"null", 5 * sizeof(Char)) == 0;
        }
        if constexpr (std::same_as<Char, char32_t>) {
            _Equals = std::memcmp(ch, U"null", 5 * sizeof(Char)) == 0;
        }

        if (!_Equals) {
            _Is.setstate(std::ios::failbit);
        }

        return *this;
    }

    template<typename ... Ts>
    basic_reader& operator>>(std::basic_string<Char, Ts...>& str) {
        _Is >> std::quoted(str);
        return *this;
    }

    template<typename T> requires (std::is_arithmetic_v<T> && !std::same_as<bool, T>)
        basic_reader& operator>>(T& number) {
        _Is >> number;
        return *this;
    }

    template<typename T> requires std::same_as<bool, T>
    basic_reader& operator>>(T& b) {
        _Is >> std::boolalpha >> b;
        return *this;
    }

    template<typename Key, typename Value>
    basic_reader& operator>>(std::pair<Key, Value>& pair) {
        char ch{};
        if constexpr (detail::is_quotable<Key>) {
            _Is >> std::quoted(pair.first);
        }
        else {
            _Is >> ch;
            if (ch != Char('"')) {
                _Is.setstate(std::ios::failbit);
                return *this;
            }

            (*this) >> pair.second;

            _Is >> ch;
            if (ch != Char('"')) {
                _Is.setstate(std::ios::failbit);
                return *this;
            }
        }

        _Is >> ch;
        if (ch != Char(':')) {
            _Is.setstate(std::ios::failbit);
            return *this;
        }

        (*this) >> pair.second;
        return *this;
    }

    template<typename Container> requires detail::is_single_container<Container>
    basic_reader& operator>>(Container& container) {
        Container _Temp{};
        char ch{};
        _Is >> ch;
        if (ch != Char('[')) {
            _Is.setstate(std::ios::failbit);
            return *this;
        }

        if (_Is.peek() != Char(']')) {
            auto it = std::back_inserter(_Temp);
            do {
                typename Container::value_type _TempValue{};
                (*this) >> _TempValue;
                *it = std::move(_TempValue);
                _Is >> ch;
                ++it;
            } while (ch == Char(','));
        }
        else {
            _Is >> ch;
        }

        if (ch != Char(']')) {
            _Is.setstate(std::ios::failbit);
            return *this;
        }

        container = std::move(_Temp);

        return *this;
    }

    template<typename Container> requires detail::is_pair_container<Container>
    basic_reader& operator>>(Container& container) {
        using pair_type = std::pair<std::remove_const_t<typename Container::value_type::first_type>, typename Container::value_type::second_type>;

        Container _Temp{};
        char ch{};
        _Is >> ch;
        if (ch != Char('{')) {
            _Is.setstate(std::ios::failbit);
            return *this;
        }

        if (_Is.peek() != Char('}')) {
            auto it = std::inserter(_Temp, _Temp.end());
            do {
                pair_type _TempValue{};
                (*this) >> _TempValue;
                *it = std::move(_TempValue);
                _Is >> ch;
                ++it;
            } while (ch == Char(','));
        }
        else {
            _Is >> ch;
        }

        if (ch != Char('}')) {
            _Is.setstate(std::ios::failbit);
            return *this;
        }

        container = std::move(_Temp);

        return *this;
    }

    operator bool() const noexcept {
        return !(_Is.bad() || _Is.fail());
    }

protected:
    std::basic_istream<Char, Traits>& _Is;
};

using reader    = basic_reader<char>;
using wreader   = basic_reader<wchar_t>;
using u8reader  = basic_reader<char8_t>;
using u16reader = basic_reader<char16_t>;
using u32reader = basic_reader<char32_t>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  FULL API
// ----------
//  Provides extended access to c++<=>json mappings.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// CONCEPTS
//

template<typename T, typename JType>
concept is_user_value = requires (const T& ut, JType& jt) {
    { jt << ut } -> std::same_as<JType&>;
}&& requires (T& ut, const JType& jt) {
    { jt >> ut } -> std::same_as<const JType&>;
};

template<typename T, typename JNull>
concept is_default_null = std::same_as<T, JNull>;

template<typename T, typename JString>
concept is_default_string = std::constructible_from<JString, T> && !std::same_as<T, std::nullptr_t>;

template<typename T, typename JNumber>
concept is_default_number = std::is_arithmetic_v<T> && !std::same_as<T, bool>;

template<typename T, typename JArray>
concept is_default_array = detail::is_single_container<T> && is_user_value<typename T::value_type, typename JArray::value_type> && !is_user_value<T, typename JArray::value_type::strin_type>;

template<typename T, typename JObject>
concept is_default_object = detail::is_pair_container<T> && is_user_value<typename T::mapped_type, typename JObject::mapped_type>;

template<typename T, typename JBool>
concept is_default_boolean = std::same_as<T, JBool>;

template<typename T, typename JSerializer>
concept is_serializable = requires (const JSerializer& js, const T& val) {
    { js(std::declval<typename JSerializer::stream_type&>(), val) } -> std::same_as<const JSerializer&>;
};

template<typename T, typename JDeserializer>
concept is_deserializable = requires (const JDeserializer& jds, T& val) {
    { jds(std::declval<typename JDeserializer::stream_type&>(), val) } -> std::same_as<const JDeserializer&>;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// ARRAY
//

template<typename JValue, typename Allocator = std::allocator<JValue>>
class basic_array {
public:
    using array_type      = std::vector<JValue, Allocator>;
    using value_type      = typename array_type::value_type;
    using allocator_type  = typename array_type::allocator_type;
    using size_type       = typename array_type::size_type;
    using difference_type = typename array_type::difference_type;
    using iterator        = typename array_type::iterator;
    using const_iterator  = typename array_type::const_iterator;

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

    array_type& get(void) noexcept {
        return _Value;
    }

    const array_type& get(void) const noexcept {
        return _Value;
    }

protected:
    array_type _Value{};
};

template<typename T, typename JValue, typename Allocator> requires is_default_array<T, basic_array<JValue, Allocator>>
inline basic_array<JValue, Allocator>& operator<<(basic_array<JValue, Allocator>& jarray, const T& container) {
    typename basic_array<JValue, Allocator>::array_type _Temp{};
    for (const auto& value : container) {
        _Temp.emplace_back() << value;
    }
    jarray.get() = std::move(_Temp);
    return jarray;
}

template<typename T, typename JValue, typename Allocator> requires is_default_array<T, basic_array<JValue, Allocator>>
inline const basic_array<JValue, Allocator>& operator>>(const basic_array<JValue, Allocator>& jarray, T& container) {
    using container_value = typename T::value_type;
    T _Temp{};
    auto it = std::back_inserter(_Temp, _Temp.end());
    for (const auto& value : jarray.get()) {
        *it = std::move(value >> container_value{});
        ++it;
    }
    container = std::move(_Temp);
    return jarray;
}

template<typename Char, typename Traits, typename JValue, typename Allocator>
inline basic_writer<Char, Traits>& operator<<(basic_writer<Char, Traits>& w, const basic_array<JValue, Allocator>& jarray) {
    return (w << jarray.get());
}

template<typename Char, typename Traits, typename JValue, typename Allocator>
inline basic_reader<Char, Traits>& operator>>(basic_reader<Char, Traits>& r, basic_array<JValue, Allocator>& jarray) {
    return (r >> jarray.get());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// OBJECT
//

template<typename JKey, typename JValue, typename Allocator = std::allocator<std::pair<const JKey, JValue>>>
class basic_object {
public:
    using object_type     = std::unordered_map<JKey, JValue, std::hash<JKey>, std::equal_to<JKey>, Allocator>;
    using key_type        = typename object_type::key_type;
    using mapped_type     = typename object_type::mapped_type;
    using allocator_type  = typename object_type::allocator_type;
    using size_type       = typename object_type::size_type;
    using difference_type = typename object_type::difference_type;
    using iterator        = typename object_type::iterator;
    using const_iterator  = typename object_type::const_iterator;

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

    object_type& get(void) noexcept {
        return _Value;
    }

    const object_type& get(void) const noexcept {
        return _Value;
    }

private:
    object_type _Value{};
};

template<typename T, typename JKey, typename JValue, typename Allocator> requires is_default_object<T, basic_object<JKey, JValue, Allocator>>
inline basic_object<JKey, JValue, Allocator>& operator<<(basic_object<JKey, JValue, Allocator>& jobject, const T& container) {
    typename basic_object<JKey, JValue, Allocator>::object_type _Temp{};
    for (const auto& [key, value] : container) {
        _Temp[typename basic_object<JKey, JValue, Allocator>::key_type(key)] << value;
    }
    jobject.get() = std::move(_Temp);
    return jobject;
}

template<typename T, typename JKey, typename JValue, typename Allocator> requires is_default_object<T, basic_object<JKey, JValue, Allocator>>
inline basic_object<JKey, JValue, Allocator>& operator<<(basic_object<JKey, JValue, Allocator>& jobject, T&& container) {
    typename basic_object<JKey, JValue, Allocator>::object_type _Temp{};
    for (auto&& [key, value] : container) {
        _Temp[typename basic_object<JKey, JValue, Allocator>::key_type(std::move(key))] << value;
    }
    jobject.get() = std::move(_Temp);
    return jobject;
}

template<typename T, typename JKey, typename JValue, typename Allocator> requires is_default_object<T, basic_object<JKey, JValue, Allocator>>
inline const basic_object<JKey, JValue, Allocator>& operator>>(const basic_object<JKey, JValue, Allocator>& jobject, T& container) {
    using container_key = std::remove_const_t<typename T::value_type::first_type>;
    using container_value = typename T::value_type::second_type;
    T _Temp{};
    auto it = std::inserter(_Temp, _Temp.end());
    for (const auto& [key, value] : jobject.get()) {
        *it = std::pair<container_key&&, container_value&&>(container_key(key), std::move(value >> container_value{}));
        ++it;
    }
    container = std::move(_Temp);
    return jobject;
}

template<typename Char, typename Traits, typename JKey, typename JValue, typename Allocator>
inline basic_writer<Char, Traits>& operator<<(basic_writer<Char, Traits>& w, const basic_object<JKey, JValue, Allocator>& jobject) {
    return (w << jobject.get());
}

template<typename Char, typename Traits, typename JKey, typename JValue, typename Allocator>
inline basic_reader<Char, Traits>& operator>>(basic_reader<Char, Traits>& r, basic_object<JKey, JValue, Allocator>& jobject) {
    return (r >> jobject.get());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// VALUE
//

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_value {
public:
    using char_type      = Char;
    using traits_type    = Traits;
    using allocator_type = Allocator;
    using null_type      = std::nullptr_t;
    using string_type    = std::basic_string<Char, Traits, Allocator>;
    using number_type    = std::double_t;
    using array_type     = basic_array<basic_value<Char, Traits, Allocator>, typename detail::rebind_alloc_t<basic_value<Char, Traits, Allocator>, Allocator>>;
    using object_type    = basic_object<string_type, basic_value<Char, Traits, Allocator>, typename detail::rebind_alloc_t<std::pair<const string_type, basic_value<Char, Traits, Allocator>>, Allocator>>;
    using boolean_type   = bool;
    using value_type     = std::variant<null_type, string_type, number_type, array_type, object_type, boolean_type>;

    bool is_null(void) const noexcept {
        return _Value.index() == 0;
    }

    null_type& to_null(void) {
        if (!this->is_null()) {
            _Value = null_type{};
        }
        return std::get<null_type>(_Value);
    }

    null_type& null(void) {
        return std::get<null_type>(_Value);
    }

    null_type& null(null_type& ref) noexcept {
        if (!this->is_null()) {
            return ref;
        }
        return std::get<null_type>(_Value);
    }

    const null_type& null(void) const {
        return std::get<null_type>(_Value);
    }

    const null_type& null(const null_type& ref) const noexcept {
        if (!this->is_null()) {
            return ref;
        }
        return std::get<null_type>(_Value);
    }

    bool is_string(void) const noexcept {
        return _Value.index() == 1;
    }

    string_type& to_string(void) {
        if (!this->is_string()) {
            _Value = string_type{};
        }
        return std::get<string_type>(_Value);
    }

    string_type& string(void) {
        return std::get<string_type>(_Value);
    }

    string_type& string(string_type& ref) noexcept {
        if (!this->is_string()) {
            return ref;
        }
        return std::get<string_type>(_Value);
    }

    const string_type& string(void) const {
        return std::get<string_type>(_Value);
    }

    const string_type& string(const string_type& ref) const noexcept {
        if (!this->is_string()) {
            return ref;
        }
        return std::get<string_type>(_Value);
    }

    bool is_number(void) const noexcept {
        return _Value.index() == 2;
    }

    number_type& to_number(void) {
        if (!this->is_number()) {
            _Value = number_type{};
        }
        return std::get<number_type>(_Value);
    }

    number_type& number(void) {
        return std::get<number_type>(_Value);
    }

    number_type& number(number_type& ref) noexcept {
        if (!this->is_number()) {
            return ref;
        }
        return std::get<number_type>(_Value);
    }

    const number_type& number(void) const {
        return std::get<number_type>(_Value);
    }

    const number_type& number(const number_type& ref) const noexcept {
        if (!this->is_number()) {
            return ref;
        }
        return std::get<number_type>(_Value);
    }

    bool is_boolean(void) const noexcept {
        return _Value.index() == 5;
    }

    boolean_type& to_boolean(void) {
        if (!this->is_boolean()) {
            _Value = boolean_type{};
        }
        return std::get<boolean_type>(_Value);
    }

    boolean_type& boolean(void) {
        return std::get<boolean_type>(_Value);
    }

    boolean_type& boolean(boolean_type& ref) noexcept {
        if (!this->is_boolean()) {
            return ref;
        }
        return std::get<boolean_type>(_Value);
    }

    const boolean_type& boolean(void) const {
        return std::get<boolean_type>(_Value);
    }

    const boolean_type& boolean(const boolean_type& ref) const noexcept {
        if (!this->is_boolean()) {
            return ref;
        }
        return std::get<boolean_type>(_Value);
    }

    bool is_array(void) const noexcept {
        return _Value.index() == 3;
    }

    array_type& to_array(void) {
        if (!this->is_array()) {
            _Value = array_type{};
        }
        return std::get<array_type>(_Value);
    }

    array_type& array(void) {
        return std::get<array_type>(_Value);
    }

    array_type& array(array_type& ref) noexcept {
        if (!this->is_array()) {
            return ref;
        }
        return std::get<array_type>(_Value);
    }

    const array_type& array(void) const {
        return std::get<array_type>(_Value);
    }

    const array_type& array(const array_type& ref) const noexcept {
        if (!this->is_array()) {
            return ref;
        }
        return std::get<array_type>(_Value);
    }

    bool is_object(void) const noexcept {
        return _Value.index() == 4;
    }

    object_type& to_object(void) {
        if (!this->is_object()) {
            _Value = object_type{};
        }
        return std::get<object_type>(_Value);
    }

    object_type& object(void) {
        return std::get<object_type>(_Value);
    }

    object_type& object(object_type& ref) noexcept {
        if (!this->is_object()) {
            return ref;
        }
        return std::get<object_type>(_Value);
    }

    const object_type& object(void) const {
        return std::get<object_type>(_Value);
    }

    const object_type& object(const object_type& ref) const noexcept {
        if (!this->is_object()) {
            return ref;
        }
        return std::get<object_type>(_Value);
    }

    value_type& get(void) noexcept {
        return _Value;
    }

    const value_type& get(void) const noexcept {
        return _Value;
    }

private:
    value_type _Value{ null_type{} };
};

template<typename Char, typename Traits, typename Allocator>
inline basic_writer<Char, Traits>& operator<<(basic_writer<Char, Traits>& w, const basic_value<Char, Traits, Allocator>& jvalue) {
    return std::visit([&](const auto& value) -> basic_writer<Char, Traits>&{
        return (w << value);
    }, jvalue.get());
}

template<typename Char, typename Traits, typename Allocator>
inline basic_reader<Char, Traits>& operator>>(basic_reader<Char, Traits>& r, basic_value<Char, Traits, Allocator>& jvalue) {
    switch (r.type()) {
    case type_id::null:    return (r >> jvalue.to_null());
    case type_id::string:  return (r >> jvalue.to_string());
    case type_id::number:  return (r >> jvalue.to_number());
    case type_id::array:   return (r >> jvalue.to_array());
    case type_id::object:  return (r >> jvalue.to_object());
    case type_id::boolean: return (r >> jvalue.to_boolean());
    default:
        throw std::bad_typeid();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// NULL
//

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_null<T, typename basic_value<Char, Traits, Allocator>::null_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::null_type(value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_null<T, typename basic_value<Char, Traits, Allocator>::null_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::null_type(std::forward<T>(value));
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_null<T, typename basic_value<Char, Traits, Allocator>::null_type>
const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    value = T(std::get<typename basic_value<Char, Traits, Allocator>::null_type>(jvalue.get()));
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::null_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    typename basic_value<Char, Traits, Allocator>::null_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::null_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    typename basic_value<Char, Traits, Allocator>::null_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::null_type>
const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    std::get<typename basic_value<Char, Traits, Allocator>::null_type>(jvalue.get()) >> value;
    return jvalue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// STRING
//

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_string<T, typename basic_value<Char, Traits, Allocator>::string_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::string_type(value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_string<T, typename basic_value<Char, Traits, Allocator>::string_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::string_type(std::forward<T>(value));
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_string<T, typename basic_value<Char, Traits, Allocator>::string_type>
const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    value = T(std::get<typename basic_value<Char, Traits, Allocator>::string_type>(jvalue.get()));
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::string_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    typename basic_value<Char, Traits, Allocator>::string_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::string_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    typename basic_value<Char, Traits, Allocator>::string_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::string_type>
const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    std::get<typename basic_value<Char, Traits, Allocator>::string_type>(jvalue.get()) >> value;
    return jvalue;
}

template<typename Char, typename Traits, typename Allocator, std::size_t N>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const Char(&value)[N]) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::string_type(value);
    return jvalue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// NUMBER
//

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_number<T, typename basic_value<Char, Traits, Allocator>::number_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::number_type(value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_number<T, typename basic_value<Char, Traits, Allocator>::number_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::number_type(std::forward<T>(value));
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_number<T, typename basic_value<Char, Traits, Allocator>::number_type>
const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    value = T(std::get<typename basic_value<Char, Traits, Allocator>::number_type>(jvalue.get()));
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::number_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    typename basic_value<Char, Traits, Allocator>::number_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::number_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    typename basic_value<Char, Traits, Allocator>::number_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::number_type>
const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    std::get<typename basic_value<Char, Traits, Allocator>::number_type>(jvalue.get()) >> value;
    return jvalue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// BOOLEN
//

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_boolean<T, typename basic_value<Char, Traits, Allocator>::boolean_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::boolean_type(value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_boolean<T, typename basic_value<Char, Traits, Allocator>::boolean_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    jvalue.get() = typename basic_value<Char, Traits, Allocator>::boolean_type(std::forward<T>(value));
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_default_boolean<T, typename basic_value<Char, Traits, Allocator>::boolean_type>
const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    value = T(std::get<typename basic_value<Char, Traits, Allocator>::boolean_type>(jvalue.get()));
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::boolean_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    typename basic_value<Char, Traits, Allocator>::boolean_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::boolean_type>
basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    typename basic_value<Char, Traits, Allocator>::boolean_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::boolean_type>
const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    std::get<typename basic_value<Char, Traits, Allocator>::boolean_type>(jvalue.get()) >> value;
    return jvalue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// ARRAY VALUE
//

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::array_type>
inline basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    typename basic_value<Char, Traits, Allocator>::array_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::array_type>
inline basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    typename basic_value<Char, Traits, Allocator>::array_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::array_type>
inline const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    std::get<typename basic_value<Char, Traits, Allocator>::array_type>(jvalue.get()) >> value;
    return jvalue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// OBJECT VALUE
//

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::object_type>
inline basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const T& value) {
    typename basic_value<Char, Traits, Allocator>::object_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::object_type>
inline basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, T&& value) {
    typename basic_value<Char, Traits, Allocator>::object_type v{};
    jvalue.get() = std::move(v << value);
    return jvalue;
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, typename basic_value<Char, Traits, Allocator>::object_type>
inline const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, T& value) {
    std::get<typename basic_value<Char, Traits, Allocator>::object_type>(jvalue.get()) >> value;
    return jvalue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// SERIALIZER
//

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_serializer {
public:
    using char_type      = Char;
    using traits_type    = Traits;
    using allocator_type = Allocator;
    using stream_type    = std::basic_ostream<Char, Traits>;

    basic_serializer(bool indentation = false, int level = 0)
        : _Indentation(indentation)
        , _Level(level)
    {
    }

    template<typename ... Ts>
    const basic_serializer& operator()(std::basic_ostream<Char, Ts...>& os, const basic_value<Char, Traits, Allocator>& value) const {
        basic_writer<Char, Traits> jw(os, _Indentation);
        if (!(jw << value)) {
            throw std::exception("Error writing value to stream");
        }
        return *this;
    }

    std::basic_string<Char, Traits, Allocator> operator()(const basic_value<Char, Traits, Allocator>& value) const {
        std::basic_ostringstream<Char, Traits, Allocator> os{};
        this->operator()(os, value);
        return os.str();
    }

    template<typename ... Ts>
    const basic_serializer& operator()(std::basic_ostream<Char, Ts...>& os, const typename basic_value<Char, Traits, Allocator>::object_type& value) const {
        basic_writer<Char, Traits> jw(os, _Indentation);
        if (!(jw << value)) {
            throw std::exception("Error writing value to stream");
        }
        return *this;
    }

    std::basic_string<Char, Traits, Allocator> operator()(const typename basic_value<Char, Traits, Allocator>::object_type& value) const {
        std::basic_ostringstream<Char, Traits, Allocator> os{};
        this->operator()(os, value);
        return os.str();
    }

    template<typename ... Ts>
    const basic_serializer& operator()(std::basic_ostream<Char, Ts...>& os, const typename basic_value<Char, Traits, Allocator>::array_type& value) const {
        basic_writer<Char, Traits> jw(os, _Indentation);
        if (!(jw << value)) {
            throw std::exception("Error writing value to stream");
        }
        return *this;
    }

    std::basic_string<Char, Traits, Allocator> operator()(const typename basic_value<Char, Traits, Allocator>::array_type& value) const {
        std::basic_ostringstream<Char, Traits, Allocator> os{};
        this->operator()(os, value);
        return os.str();
    }

    template<typename T, typename ... Ts> requires is_user_value<T, basic_value<Char, Traits, Allocator>>
    const basic_serializer& operator()(std::basic_ostream<Char, Ts...>& os, const T& value) const {
        basic_value<Char, Traits, Allocator> v{};
        this->operator()(os, v << value);
        return *this;
    }

    template<typename T> requires is_user_value<T, basic_value<Char, Traits, Allocator>>
    std::basic_string<Char, Traits, Allocator> operator()(const T& value) const {
        std::basic_ostringstream<Char, Traits, Allocator> os{};
        this->operator()(os, value);
        return os.str();
    }

    void indentation(bool indentation) noexcept {
        _Indentation = indentation;
    }

    bool indentation(void) const noexcept {
        return _Indentation;
    }

    void level(int level) noexcept {
        _Level = level;
    }

    int level(void) const noexcept {
        return _Level;
    }

private:
    bool _Indentation{ false };
    int  _Level{ 0 };
};

template<typename T, typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>> requires is_serializable<T, basic_serializer<Char, Traits, Allocator>>
inline std::optional<std::exception> serialize(std::basic_ostream<Char, Traits>& os, const T& value, bool indentation = false, int level = 0) noexcept {
    try {
        basic_serializer<Char, Traits, Allocator> js{ indentation, level };
        js(os, value);
        return std::optional<std::exception>{};
    }
    catch (std::exception e) {
        return e;
    }
}

template<typename T, typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>> requires is_serializable<T, basic_serializer<Char, Traits, Allocator>>
inline std::optional<std::exception> serialize(std::basic_string<Char, Traits, Allocator>& str, const T& value, bool indentation = false, int level = 0) noexcept {
    try {
        basic_serializer<Char, Traits, Allocator> js{ indentation, level };
        str = std::move(js(value));
        return std::optional<std::exception>{};
    }
    catch (std::exception e) {
        return e;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// DESERIALIZER
//

template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
class basic_deserializer {
public:
    using char_type = Char;
    using traits_type = Traits;
    using allocator_type = Allocator;
    using stream_type = std::basic_istream<Char, Traits>;

    template<typename ... Ts>
    const basic_deserializer& operator()(std::basic_istream<Char, Ts...>& is, basic_value<Char, Traits, Allocator>& value) const {
        basic_reader<Char, Traits> jr(is);
        if (!(jr >> value)) {
            throw std::exception("Error reading value to stream");
        }
        return *this;
    }

    template<typename ... Ts>
    const basic_deserializer& operator()(const std::basic_string<Char, Ts...>& str, basic_value<Char, Traits, Allocator>& value) const {
        std::basic_istringstream<Char, Ts...> is{ str };
        return this->operator()(is, value);
    }

    template<typename ... Ts>
    const basic_deserializer& operator()(std::basic_istream<Char, Ts...>& is, typename basic_value<Char, Traits, Allocator>::object_type& value) const {
        basic_reader<Char, Traits> jr(is);
        if (!(jr >> value)) {
            throw std::exception("Error reading value to stream");
        }
        return *this;
    }

    template<typename ... Ts>
    const basic_deserializer& operator()(const std::basic_string<Char, Ts...>& str, typename basic_value<Char, Traits, Allocator>::object_type& value) const {
        std::basic_istringstream<Char, Ts...> is{ str };
        return this->operator()(is, value);
    }

    template<typename ... Ts>
    const basic_deserializer& operator()(std::basic_istream<Char, Ts...>& is, typename basic_value<Char, Traits, Allocator>::array_type& value) const {
        basic_reader<Char, Traits> jr(is);
        if (!(jr >> value)) {
            throw std::exception("Error reading value to stream");
        }
        return *this;
    }

    template<typename ... Ts>
    const basic_deserializer& operator()(const std::basic_string<Char, Ts...>& str, typename basic_value<Char, Traits, Allocator>::array_type& value) const {
        std::basic_istringstream<Char, Ts...> is{ str };
        return this->operator()(is, value);
    }

    template<typename T, typename ... Ts> requires is_user_value<T, basic_value<Char, Traits, Allocator>>
    const basic_deserializer& operator()(std::basic_istream<Char, Ts...>& is, T& value) const {
        basic_value<Char, Traits, Allocator> v{};
        basic_reader<Char, Traits> jr(is);
        if (!(jr >> v)) {
            throw std::exception("Error reading value to stream");
        }
        v >> value;
        return *this;
    }

    template<typename T, typename ... Ts> requires is_user_value<T, basic_value<Char, Traits, Allocator>>
    const basic_deserializer& operator()(const std::basic_string<Char, Ts...>& str, T& value) const {
        std::basic_istringstream<Char, Ts...> is{ str };
        return this->operator()(is, value);
    }
};

template<typename T, typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>> requires is_deserializable<T, basic_deserializer<Char, Traits, Allocator>>
inline std::optional<std::exception> deserialize(std::basic_istream<Char, Traits>& is, T& value) noexcept {
    try {
        basic_deserializer<Char, Traits, Allocator>{}(is, value);
        return std::optional<std::exception>{};
    }
    catch (std::exception e) {
        return e;
    }
}

template<typename T, typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>> requires is_deserializable<T, basic_deserializer<Char, Traits, Allocator>>
inline std::optional<std::exception> deserialize(const std::basic_string<Char, Traits, Allocator>& str, T& value) noexcept {
    try {
        basic_deserializer<Char, Traits, Allocator>{}(str, value);
        return std::optional<std::exception>{};
    }
    catch (std::exception e) {
        return e;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// ALIASES
//

using value           = basic_value<char>;
using null            = typename value::null_type;
using string          = typename value::string_type;
using number          = typename value::number_type;
using array           = typename value::array_type;
using object          = typename value::object_type;
using boolean         = typename value::boolean_type;
using serializer      = basic_serializer<char>;
using deserializer    = basic_deserializer<char>;

using wvalue          = basic_value<wchar_t>;
using wnull           = typename value::null_type;
using wstring         = typename value::string_type;
using wnumber         = typename value::number_type;
using warray          = typename value::array_type;
using wobject         = typename value::object_type;
using wboolean        = typename value::boolean_type;
using wserializer     = basic_serializer<wchar_t>;
using wdeserializer   = basic_deserializer<wchar_t>;

using u8value         = basic_value<char8_t>;
using u8null          = typename value::null_type;
using u8string        = typename value::string_type;
using u8number        = typename value::number_type;
using u8array         = typename value::array_type;
using u8object        = typename value::object_type;
using u8boolean       = typename value::boolean_type;
using u8serializer    = basic_serializer<char8_t>;
using u8deserializer  = basic_deserializer<char8_t>;

using u16value        = basic_value<char16_t>;
using u16null         = typename value::null_type;
using u16string       = typename value::string_type;
using u16number       = typename value::number_type;
using u16array        = typename value::array_type;
using u16object       = typename value::object_type;
using u16boolean      = typename value::boolean_type;
using u16serializer   = basic_serializer<char16_t>;
using u16deserializer = basic_deserializer<char16_t>;

using u32value        = basic_value<char32_t>;
using u32null         = typename value::null_type;
using u32string       = typename value::string_type;
using u32number       = typename value::number_type;
using u32array        = typename value::array_type;
using u32object       = typename value::object_type;
using u32boolean      = typename value::boolean_type;
using u32serializer   = basic_serializer<char32_t>;
using u32deserializer = basic_deserializer<char32_t>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  EXTENDED API
// --------------
//  Provides extensions to the FULL API
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// UNIQUE_PTR
//

template<typename T, typename Deleter, typename Char, typename Traits, typename Allocator> requires is_user_value<T, basic_value<Char, Traits, Allocator>>
inline basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const std::unique_ptr<T, Deleter>& ptr) {
    if (!ptr) {
        jvalue.to_null();
        return jvalue;
    }
    return jvalue << (*ptr);
}

template<typename T, typename Deleter, typename Char, typename Traits, typename Allocator> requires is_user_value<T, basic_value<Char, Traits, Allocator>>
inline const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, std::unique_ptr<T, Deleter>& ptr) {
    if (jvalue.is_null()) {
        ptr.reset();
        return jvalue;
    }
    if (!ptr) {
        ptr.release(new T());
    }
    return jvalue >> (*ptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// SHARED_PTR
//

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, basic_value<Char, Traits, Allocator>>
inline basic_value<Char, Traits, Allocator>& operator<<(basic_value<Char, Traits, Allocator>& jvalue, const std::shared_ptr<T>& ptr) {
    if (!ptr) {
        jvalue.to_null();
        return jvalue;
    }
    return jvalue << (*ptr);
}

template<typename T, typename Char, typename Traits, typename Allocator> requires is_user_value<T, basic_value<Char, Traits, Allocator>>
inline const basic_value<Char, Traits, Allocator>& operator>>(const basic_value<Char, Traits, Allocator>& jvalue, std::shared_ptr<T>& ptr) {
    if (jvalue.is_null()) {
        ptr.reset();
        return jvalue;
    }
    if (!ptr) {
        ptr.release(new T());
    }
    return jvalue >> (*ptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RW_JSON_NAMESPACE_END
RW_NAMESPACE_END

#endif
