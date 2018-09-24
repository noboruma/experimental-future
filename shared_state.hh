#pragma once

namespace experimental
{
    namespace impl
    {
        template<typename T, bool isReference>
        struct shared_state_spe{};

        template<typename T>
        struct shared_state : public shared_state_spe<T, std::is_reference<T>::value> {};

        template<typename T>
        struct shared_state_spe<T, false>
        {
            shared_state_spe<T,false>() = default;
            shared_state_spe<T,false>(shared_state_spe<T,false> && o) noexcept = delete;
            shared_state_spe<T,false>(shared_state_spe<T,false> const& o) noexcept = delete;
            shared_state_spe<T,false>& operator=(shared_state_spe<T,false> && o) noexcept = delete;
            shared_state_spe<T,false>& operator=(shared_state_spe<T,false> const& o) noexcept = delete;

            void wait() const
            {
                std::unique_lock<std::mutex> l(_m);
                _cv.wait(l, [this] () -> bool
                         {
                             return _isReady;
                         });
            }

            template< class Rep, class Period >
            bool wait_for( const std::chrono::duration<Rep,Period>& timeout_duration) const
            {
                std::unique_lock<std::mutex> l(_m);
                return _cv.wait_for(l,
                                    timeout_duration,
                                    [this] () -> bool
                                    {
                                        return _isReady;
                                    });
            }

            template< class Clock, class Duration >
            bool wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) const
            {
                std::unique_lock<std::mutex> l(_m);
                return _cv.wait_until(l,
                                      timeout_time,
                                      [this] () -> bool
                                      {
                                          return _isReady;
                                      });
            }

            bool isReady() const
            {
                return _isReady;
            }

            void set_value(T &&t)
            {
                std::unique_lock<std::mutex> l(_m_thens);
                {
                    std::unique_lock<std::mutex> l(_m);
                    _isReady = true;
                    std::swap(_state, t);
                }
                _cv.notify_all();

                if (!_thens.empty()) {
                    for(auto &then : _thens) {
                        then(_state, nullptr);
                    }
                }
            }

            void set_exception(std::exception_ptr p)
            {
                std::unique_lock<std::mutex> l(_m_thens);
                {
                    std::unique_lock<std::mutex> l(_m);
                    _isReady = true;
                    _error = p;
                }
                if (!_thens.empty()) {
                    char dummy[sizeof(T)];
                    for(auto &then : _thens) {
                        then(*reinterpret_cast<T*>(dummy), std::move(p));
                    }
                }
                _cv.notify_all();
            }

            T get_value()
            {
                if(_error) {
                    std::rethrow_exception(_error);
                }
                return std::move(_state);
            }

            void set_then(std::function<void(const T&, std::exception_ptr)> f)
            {
                std::unique_lock<std::mutex> l(_m_thens);
                if(_isReady) {
                    f(_state, _error);
                }
                _thens.push_back(std::move(f));
            }

            private:
            mutable std::condition_variable _cv;
            T _state;
            std::exception_ptr _error;
            bool _isReady = false;
            mutable std::mutex _m, _m_thens;
            std::list<std::function<void(T const&, std::exception_ptr)>> _thens;
        };

        template<typename T>
        struct shared_state_spe<T, true>
        {
            using ref_type   = T;
            using value_type = typename std::remove_reference<T>::type;

            shared_state_spe<T,true>() = default;
            shared_state_spe<T,true>(shared_state_spe<T,true> && o) noexcept = delete;
            shared_state_spe<T,true>(shared_state_spe<T,true> const& o) noexcept = delete;
            shared_state_spe<T,true>& operator=(shared_state_spe<T,true> && o) noexcept = delete;
            shared_state_spe<T,true>& operator=(shared_state_spe<T,true> const& o) noexcept = delete;

            void wait() const
            {
                std::unique_lock<std::mutex> l(_m);
                _cv.wait(l, [this] () -> bool
                         {
                             return _isReady;
                         });

            }

            template< class Rep, class Period >
            bool wait_for( const std::chrono::duration<Rep,Period>& timeout_duration) const
            {
                std::unique_lock<std::mutex> l(_m);
                return _cv.wait_for(l,
                                    timeout_duration,
                                    [this] () -> bool
                                    {
                                        return _isReady;
                                    });
            }

            template< class Clock, class Duration >
            bool wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) const
            {
                std::unique_lock<std::mutex> l(_m);
                return _cv.wait_until(l,
                                      timeout_time,
                                      [this] () -> bool
                                      {
                                          return _isReady;
                                      });
            }

            bool isReady() const
            {
                return _isReady;
            }

            void set_value(value_type &t)
            {
                std::unique_lock<std::mutex> l(_m_thens);
                {
                    std::unique_lock<std::mutex> l(_m);
                    _isReady = true;
                    _state = &t;
                }
                _cv.notify_all();
                if (!_thens.empty()) {
                    for(auto &then : _thens) {
                        then(*_state, nullptr);
                    }
                }
            }

            void set_exception(std::exception_ptr p)
            {
                std::unique_lock<std::mutex> l(_m_thens);
                {
                    std::unique_lock<std::mutex> l(_m);
                    _isReady = true;
                    _error = p;
                }
                if (!_thens.empty()) {
                    char dummy[sizeof(value_type)];
                    for(auto &then : _thens) {
                        then(*reinterpret_cast<value_type*>(dummy), std::move(p));
                    }
                }

                _cv.notify_all();
            }

            value_type& get_value()
            {
                if(_error) {
                    std::rethrow_exception(_error);
                }
                return *_state;
            }

            void set_then(std::function<void(value_type &, std::exception_ptr)> f)
            {
                std::unique_lock<std::mutex> l(_m_thens);
                if(_isReady) {
                    if(_error) {
                        char dummy[sizeof(value_type)];
                        f(*reinterpret_cast<value_type*>(dummy), _error);
                    } else{
                        f(*_state, nullptr);
                    }
                }
                _thens.emplace_back(std::move(f));
            }

            private:
            mutable std::condition_variable _cv;
            value_type *_state=nullptr;
            std::exception_ptr _error;
            bool _isReady = false;
            mutable std::mutex _m, _m_thens;
            std::list<std::function<void(value_type &, std::exception_ptr)>> _thens;
        };

        template<>
        struct shared_state_spe<void, false>
        {
            shared_state_spe<void,false>() = default;
            shared_state_spe<void,false>(shared_state_spe<void,false> && o) noexcept = delete;
            shared_state_spe<void,false>(shared_state_spe<void,false> const& o) noexcept = delete;
            shared_state_spe<void,false>& operator=(shared_state_spe<void,false> && o) noexcept = delete;
            shared_state_spe<void,false>& operator=(shared_state_spe<void,false> const& o) noexcept = delete;

            void wait() const
            {
                std::unique_lock<std::mutex> l(_m);
                return _cv.wait(l, [this] () -> bool
                                {
                                    return _isReady;
                                });
            }

            template< class Rep, class Period >
            bool wait_for( const std::chrono::duration<Rep,Period>& timeout_duration) const
            {
                std::unique_lock<std::mutex> l(_m);
                return _cv.wait_for(l,
                                    timeout_duration,
                                    [this] () -> bool
                                    {
                                        return _isReady;
                                    });
            }

            template< class Clock, class Duration >
            bool wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) const
            {
                std::unique_lock<std::mutex> l(_m);
                return _cv.wait_until(l, timeout_time,
                                      [this] () -> bool
                                      {
                                          return _isReady;
                                      });
            }

            bool isReady() const
            {
                return _isReady;
            }

            void set_value()
            {
                std::unique_lock<std::mutex> l(_m_thens);
                {
                    std::unique_lock<std::mutex> l(_m);
                    _isReady = true;
                }
                _cv.notify_all();
                if (!_thens.empty()) {
                    for(auto &then : _thens) {
                        then(nullptr);
                    }
                }

            }

            void set_exception(std::exception_ptr p)
            {
                std::unique_lock<std::mutex> l(_m_thens);
                {
                    std::unique_lock<std::mutex> l(_m);
                    _isReady = true;
                    _error = p;
                }
                if (!_thens.empty()) {
                    for(auto &then : _thens) {
                        then(std::move(p));
                    }
                }
                _cv.notify_all();
            }

            void get_value()
            {
                if(_error) {
                    std::rethrow_exception(_error);
                }
                return;
            }

            void set_then(std::function<void(std::exception_ptr)> f)
            {
                std::unique_lock<std::mutex> l(_m_thens);
                if(_isReady) {
                    f(_error);
                }
                _thens.emplace_back(std::move(f));
            }

            private:
            mutable std::condition_variable _cv;
            std::exception_ptr _error;
            bool _isReady = false;
            mutable std::mutex _m, _m_thens;
            std::list<std::function<void(std::exception_ptr)>> _thens;
        };
    }//!impl
}//!experimental
