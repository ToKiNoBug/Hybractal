#include <concepts>
#include <tuple>

namespace hypractal {

namespace detail {
template<auto V>
struct type_wrapper
{
    static constexpr auto value = V;
};
}  // namespace detail

template<auto... Vals>
struct array_t
{
    using values = std::tuple<detail::type_wrapper<Vals>...>;
};

template<typename arr>
constexpr size_t array_size_v = std::tuple_size_v<typename arr::values>;

template<size_t index, typename arr>
requires(index < array_size_v<arr>) constexpr auto array_at_v = std::tuple_element_t<index, typename arr::values>::value;

namespace detail {
template<typename f, typename o, size_t... Is>
consteval auto array_reorder_impl(std::index_sequence<Is...>)
{
    return std::declval<array_t<array_at_v<array_at_v<Is, o>, f>...>>();
}

template<typename f, typename o, size_t... Is>
consteval auto array_reorder()
{
    return array_reorder_impl<f, o>(std::make_index_sequence<array_size_v<o>>{});
}
}  // namespace detail

template<typename f, typename o>
using array_reorder_t = decltype(detail::array_reorder<f, o>());

}  // namespace hypractal
