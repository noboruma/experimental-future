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
                using value_type = T;
            };

            template <template<typename> class T, typename A>
            struct unwrap_future<T<A>>
            {
                using value_type = typename std::conditional<is_future, A, T<A>>::type;
            };

            template <template<typename> class T, typename A>
            struct unwrap_future<T<A>&> 
            {
                using value_type = typename std::conditional<is_future, A, T<A>>::type;
            };

        }//!utils
    }//!impl
}//!experimental
