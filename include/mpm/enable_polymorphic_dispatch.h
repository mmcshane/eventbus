#pragma once

#include "mpm/detail/typelist.h"

namespace mpm
{
    namespace detail
    {
        /// root class for polymorphic event hierarchies
        class event
        {
          protected:
            //don't dispatch as this type - it's an internal
            //implementation detail
            using dispatch_as = null_t;

            virtual ~event()
            {
            }
        };

        /// forward decl from eventbus.h
        template <typename E> struct dispatch_typelist;
    }


    //! Publicly inherit from this class to obtain polymorphic
    //! event delivery.
    template <typename T, typename Base=detail::event>
    class enable_polymorphic_dispatch : public Base
    {
      protected:

        // \internal
        // Couldn't use a typelist based on std::tuple here because
        // std::tuple_cat won't work with an incomplete type, and the
        // derived event types are incomplete when decltype(tuple_cat)
        // would be used


#       ifdef DOCS
        using dispatch_as = implementation-defined;
#       else
        template <typename E> friend class detail::dispatch_typelist;
        using dispatch_as = detail::typelist<T, typename Base::dispatch_as>;
#       endif
    };
}
