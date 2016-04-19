#pragma once

namespace mpm
{

    //! A type used to indicate an empty typelist tail. I.e. the end marker
    //! of a typelist.
    struct null_t {};

    //! Typelist a la [Alexandrescu](http://www.drdobbs.com/generic-programmingtypelists-and-applica/184403813)
    template <typename Head, typename Tail=null_t>
    struct typelist
    {
        //! The first element, a non-typelist by convention
        using head = Head;

        //! The second element, can be another typelist
        using tail = Tail;
    };


    namespace detail
    {
        template <typename F, typename TL>
        struct for_each_helper
        {
            void operator()(F f) const
            {
                f.template operator()<typename TL::head>();
                for_each_helper<F, typename TL::tail>()(f);
            }
        };

        template <typename F>
        struct for_each_helper<F, null_t>
        {
            void operator()(const F&){}
        };
    }


    //! Iterate over a typelist at runtime.
    //! The supplied functor, f, is invoked for each element in the typlist TL
    //!
    //! \tparam TL A typelist
    //! \tparam F A FunctionObject
    //! \param f A functor that will be called as f.template operator()\<T>()
    //!          where T is an element in TL. This functor will be invoked once
    //!          for each element of TL
    //! \relates typelist
    template <typename TL, typename F>
    void for_each_type(const F& f)
    {
        detail::for_each_helper<F, TL>()(f);
    }
}
