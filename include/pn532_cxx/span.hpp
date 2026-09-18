#pragma once

#include <cstddef>
#include <type_traits>

#if __cplusplus >= 202002L
  #include <span>
#endif

namespace pn532 {

namespace detail {
    template <typename From, typename To>
    using is_allowed_pointer_conversion = std::is_convertible<From(*)[], To(*)[]>;

    template <typename Container, typename ElementType, typename = void>
    struct is_compatible_container : std::false_type {};

    template <typename Container, typename ElementType>
    struct is_compatible_container<Container, ElementType, std::void_t<
        decltype(std::declval<Container&>().data()),
        decltype(std::declval<Container&>().size())
    >> : is_allowed_pointer_conversion<
            typename std::remove_pointer<decltype(std::declval<Container&>().data())>::type, 
            ElementType
         > {};
} // namespace detail

template<typename T>
class span {
public:
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = pointer;

    constexpr span() noexcept : _data(nullptr), _size(0) {}

    constexpr span(pointer data, size_type size) noexcept : _data(data), _size(size) {}

    constexpr span(pointer first, pointer last) noexcept : _data(first), _size(last - first) {}

    template<std::size_t N>
    constexpr span(element_type (&arr)[N]) noexcept : _data(arr), _size(N) {}

    template<typename Container, typename = typename std::enable_if<
        !std::is_same<typename std::decay<Container>::type, span>::value &&
        detail::is_compatible_container<Container, element_type>::value
    >::type>
    constexpr span(Container& c) noexcept : _data(c.data()), _size(c.size()) {}

    template<typename Container, typename = typename std::enable_if<
        !std::is_same<typename std::decay<Container>::type, span>::value &&
        detail::is_compatible_container<const Container, element_type>::value
    >::type>
    constexpr span(const Container& c) noexcept : _data(c.data()), _size(c.size()) {}

    template<typename U, typename = typename std::enable_if<
        detail::is_allowed_pointer_conversion<U, element_type>::value
    >::type>
    constexpr span(const span<U>& other) noexcept : _data(other.data()), _size(other.size()) {}

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
    constexpr span(std::span<T> s) noexcept : _data(s.data()), _size(s.size()) {}
    constexpr operator std::span<T>() const noexcept { return std::span<T>(_data, _size); }
#endif

    constexpr pointer data() const noexcept { return _data; }
    constexpr size_type size() const noexcept { return _size; }
    constexpr size_type size_bytes() const noexcept { return _size * sizeof(element_type); }
    constexpr bool empty() const noexcept { return _size == 0; }

    constexpr reference operator[](size_type idx) const { return _data[idx]; }
    constexpr reference front() const { return _data[0]; }
    constexpr reference back() const { return _data[_size - 1]; }

    constexpr iterator begin() const noexcept { return _data; }
    constexpr iterator end() const noexcept { return _data + _size; }

    constexpr span subspan(size_type offset, size_type count = static_cast<size_type>(-1)) const {
        if (offset > _size) {
            return span();
        }
        if (count == static_cast<size_type>(-1) || count > _size - offset) {
            count = _size - offset;
        }
        return span(_data + offset, count);
    }

private:
    pointer _data;
    size_type _size;
};

#if __cplusplus >= 201703L
template<typename T, std::size_t N>
span(T (&)[N]) -> span<T>;

template<typename Container>
span(Container&) -> span<typename Container::value_type>;

template<typename Container>
span(const Container&) -> span<const typename Container::value_type>;
#endif

} // namespace pn532
