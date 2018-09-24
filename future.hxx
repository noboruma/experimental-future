#pragma once

namespace experimental {
    namespace {
        /*
         * Calls either:
         * - return s.get_value()
         * - s.get_value(); return;
         */
        template<typename U>
        struct value_getter
        {
            static U apply(impl::shared_state<U> &s){
                return std::forward<U>(s.get_value());
            }
        };

        template<>
        struct value_getter<void> {
            static void apply(impl::shared_state<void> &s){
                s.get_value();
                return;
            }
        };
    }

    template<typename T>
    promise<T>::promise()
    : _state(std::make_shared<impl::shared_state<T>>())
    {
    }

    template<typename T>
    void promise<T>::set_value(T &&t)
    {
        _state->set_value(std::forward<T>(t));
    }

    template<typename T>
    void promise<T>::set_exception(std::exception_ptr p)
    {
        _state->set_exception(p);
    }

    template<typename T>
    future<T> promise<T>::get_future()
    {
        return future<T>(_state);
    }

    template<typename T>
    void promise<T>::swap(promise<T> &other) noexcept
    {
        std::swap(_state, other._state);
    }

    template<typename T>
    future<T>::future() noexcept
    {
    }

    template<typename T>
    future<T>::future(future<T> &&f) noexcept
    : _state(std::move(f._state))
    {
    }

    template<typename T>
    T future<T>::get()
    {
        _state->wait();
        return value_getter<T>::apply(*_state);
    }

    template<typename T>
    void future<T>::wait() const
    {
        _state->wait();
    }

    template<typename T>
    template< class Rep, class Period >
    future_status future<T>::wait_for( const std::chrono::duration<Rep,Period>& timeout_duration) const
    {
        return _state->wait_for(timeout_duration) ? future_status::ready : future_status::timeout;
    }

    template<typename T>
    template< class Clock, class Duration >
    future_status future<T>::wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) const
    {
        return _state->wait_until(timeout_time) ? future_status::ready : future_status::timeout;
    }

    template<typename T>
    bool future<T>::valid() const noexcept
    {
        return static_cast<bool>(_state);
    }

    template<typename T>
    bool future<T>::is_ready() const
    {
        return valid() && _state->isReady();
    }

    template<typename T>
    template<typename F>
    auto future<T>::then(F && func) -> future<typename impl::utils::unwrap_future<decltype(func())>::value_type>
    {
        using RetTraits = impl::utils::unwrap_future<decltype(func())>;
        using RetType = typename RetTraits::value_type;
        auto state = std::make_shared<impl::shared_state<RetType>>();
        _state->set_then([state=state, func=std::move(func)](std::exception_ptr &&p) mutable -> void
                         {
                             if(p) {
                                 state->set_exception(p);
                             } else {
                                 try {
                                     impl::utils::apply_continuation<!std::is_same<RetType, void>::value, RetTraits>::invoke(*state,
                                                                                                                             func);
                                 } catch(...) {
                                     state->set_exception(std::current_exception());
                                 }
                             }
                         });
        return future<RetType>(std::move(state));
    }

    template<typename T>
    template<typename F>
    auto future<T>::then(F && func) -> future<typename impl::utils::unwrap_future<decltype(func(std::declval<T>()))>::value_type>
    {
        using RetTraits = impl::utils::unwrap_future<decltype(func(std::declval<T>()))>;
        using RetType = typename RetTraits::value_type;
        auto state = std::make_shared<impl::shared_state<RetType>>();
        _state->set_then([state=state, func=std::move(func)](T const&t, std::exception_ptr &&p) mutable -> void
                         {
                             if(p) {
                                 state->set_exception(p);
                             } else {
                                 try {
                                     impl::utils::apply_continuation<!std::is_same<RetType, void>::value, RetTraits>::invoke(*state,
                                                                                                                             func,
                                                                                                                             t);
                                 } catch(...) {
                                     state->set_exception(std::current_exception());
                                 }
                             }
                         });
        return future<RetType>(std::move(state));
    }

    template<typename T>
    future<T>::future(std::shared_ptr<impl::shared_state<T>> state)
    : _state(std::move(state))
    {}


    promise<void>::promise()
    : _state(std::make_shared<impl::shared_state<void>>())
    {
    }

    void promise<void>::set_value() {
        _state->set_value();
    }

    void promise<void>::set_exception(std::exception_ptr p) {
        _state->set_exception(p);
    }

    future<void> promise<void>::get_future()
    {
        return future<void>(_state);
    }

    void promise<void>::swap(promise<void> &other) noexcept
    {
        std::swap(_state, other._state);
    }

}//!experimental
