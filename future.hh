#pragma once

#include <algorithm>
#include <atomic>
#include <future>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <type_traits>

#include "future_traits.hh"
#include "shared_state.hh"

namespace experimental
{
    using future_status = std::future_status;

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

        private:
            std::shared_ptr<impl::shared_state<T>> _state;
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

        private:
            std::shared_ptr<impl::shared_state<void>> _state;
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
             * Relies on SFINAE to handle 'T == void' special case
             */
            template<typename F>
            auto then(F && func) -> future<typename impl::utils::unwrap_future<decltype(func())>::value_type>;

            template<typename F>
            auto then(F && func) -> future<typename impl::utils::unwrap_future<decltype(func(std::declval<T>()))>::value_type>;

        private:
            future(std::shared_ptr<impl::shared_state<T>> state);

        private:
            std::shared_ptr<impl::shared_state<T>> _state;

            friend struct experimental::promise<T>;
            template<typename>
            friend class experimental::future;
    };
}//!experimental

#include "future.hxx"
