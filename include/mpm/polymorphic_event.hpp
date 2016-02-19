#pragma once

#include "mpm/typelist.hpp"

namespace mpm
{
    namespace detail
    {
        /// root class for polymorphic_event hierarchies
        class event
        {
          public:
            //don't dispatch as this type - it's an internal
            //implementation detail
            using dispatch_as = null_t;

          protected:
            virtual ~event()
            {
            }
        };
    }


    //! publicly inherit from this class to obtain polymorphic
    //! event delivery. Other possible name for this class:
    //! enable_polymorphic_dispatch (a la enable_shared_from_this)
    template <typename T, typename Base=detail::event>
    class polymorphic_event : public Base
    {
      public:

        // N.B. Couldn't use a typelist based on std::tuple here because
        // std::tuple_cat won't work with an incomplete type, and the
        // derived event types are incomplete when decltype(tuple_cat)
        // would be used

        using dispatch_as = typelist<T, typename Base::dispatch_as>;
    };
}
