#pragma once

#include <type_traits>

namespace experimental {

    template<typename T>
    class future;

    namespace impl
    {
        namespace utils
        {
            /*
             * Future unwrapping
             */
            template <typename T>
            struct unwrap_future
            {
                const static bool is_future = false;
                using value_type = T;
            };

            template <template<typename> class T, typename A>
            struct unwrap_future<T<A>>
            {
                const static bool is_future = std::is_same<T<A>, experimental::future<A>>::value;
                using value_type = typename std::conditional<is_future, A, T<A>>::type;
            };

            template <template<typename> class T, typename A>
            struct unwrap_future<T<A>&>
            {
                const static bool is_future = std::is_same<T<A>&, experimental::future<A>&>::value;
                using value_type = typename std::conditional<is_future, A, T<A>>::type;
            };

            /*
             * Calls a given function either:
             * - func(...)
             * - func(...).get()
             */
            template<bool retFuture>
            struct call_get_if_returns_future {};

            template<>
            struct call_get_if_returns_future<true>
            {
                template<typename F, typename ...Arg>
                static auto invoke(F &&func, Arg &&... arg) -> typename unwrap_future<decltype(func(arg...))>::value_type
                {
                    return func(std::forward<Arg>(arg)...).get();
                }
            };

            template<>
            struct call_get_if_returns_future<false>
            {
                template<typename F, typename ... Arg>
                static auto invoke(F &&func, Arg &&...arg) -> typename unwrap_future<decltype(func(arg...))>::value_type
                {
                    return func(std::forward<Arg>(arg)...);
                }
            };

            /*
             * Set a given promise either:
             * - f.set_value(func(...))
             * - func(...); f.set_value();
             */
            template<bool passArgument, typename Traits>
            struct apply_continuation {};

            template<typename Traits>
            struct apply_continuation<true, Traits>
            {
                template<typename Future, typename Function, typename ... Arg>
                static auto invoke(Future &&future, Function && func, Arg &&... arg) -> void
                {
                    future.set_value(call_get_if_returns_future<Traits::is_future>::invoke(std::forward<Function>(func),std::forward<Arg>(arg)...));
                }
            };

            template<typename Traits>
            struct apply_continuation<false, Traits>
            {
                template<typename Future, typename Function, typename ... Arg>
                static auto invoke(Future &&future, Function && func, Arg &&... arg) -> void
                {
                    call_get_if_returns_future<Traits::is_future>::invoke(std::forward<Function>(func),std::forward<Arg>(arg)...);
                    future.set_value();
                }
            };
        }//!utils
    }//!impl
}//!experimental
