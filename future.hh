#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <type_traits>

#include "future_traits.hh"

namespace experimental {

    template<typename T>
    class promise
    {
        public:
            promise();
            promise(promise<T> && other) noexcept = default;

            promise<T>& operator=(promise<T> && other) noexcept = default;
            promise<T>& operator=(promise<T> const &other) = delete;

            void set_value(T &&t);
            void set_exception(std::exception_ptr p);

            future<T> get_future();

            void swap(promise<T> &other) noexcept;
    };

    template<>
    class promise<void>
    {
        public:
            promise();
            promise(promise<void> && other) noexcept = default;

            promise<void>& operator=(promise<void> && other) noexcept = default;
            promise<void>& operator=(promise<void> const &other) = delete;

            void set_value();
            void set_exception(std::exception_ptr p);

            future<void> get_future();

            void swap(promise<void> &other) noexcept;
    };

    template<typename T>
    class future
    {
        public:
            future() noexcept;
            future(future<T> &&f) noexcept;
            future(future<T> const &f) = delete;

            future<T>& operator=(future<T> && other) noexcept = default;
            future<T>& operator=(future<T> const &other) = delete;

            T get();

            void wait() const;

            template< class Rep, class Period >
            future_status wait_for( const std::chrono::duration<Rep,Period>& timeout_duration) const;

            template< class Clock, class Duration >
            future_status wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) const;

            bool valid() const noexcept;

            bool is_ready() const;

            /*
             * use SFINAE to handle "void" case
             */
            template<typename F>
            auto then(F && func) -> future<typename impl::utils::unwrap_future<decltype(func())>::value_type>;

            template<typename F>
            auto then(F && func) -> future<typename impl::utils::unwrap_future<decltype(func(std::declval<T>()))>::value_typ↪e>;
    };

}//!experimental
