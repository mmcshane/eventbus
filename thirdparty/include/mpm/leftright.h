#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <mutex>
#include <thread>
#include <type_traits>


#ifndef MPM_LEFTRIGHT_CACHE_LINE_SIZE
#   define MPM_LEFTRIGHT_CACHE_LINE_SIZE 64
#endif

namespace mpm
{
    //! \defgroup Concepts
    //! Concept is a term that describes a named set of requirements for a type.

    //! \defgroup ReaderRegistry
    //! \ingroup Concepts
    //! \{
    //!
    //! Keeps track of active readers such that it can efficiently
    //! indicate whether there are any active readers when queried.
    //!
    //! \par Extends
    //! DefaultConstructible
    //!
    //! \par Requirements
    //! Given:\n
    //! R, an implementation of the ReaderRegistry concept \n
    //! r, an instance of R
    //!
    //! |Expression  | Requirements                                             | Return type  |
    //! |:-----------|:---------------------------------------------------------|:-------------|
    //! | R()        | R is default constructible.                              | R            |
    //! | r.arrive() | Notes the arival of a reader. Wait-free and noexcept.    | void         |
    //! | r.depart() | Notes the departure of a reader. Wait-free and noexcept. | void         |
    //! | r.empty()  | const and noexcept.    | true if there are no readers; false otherwise. |
    //! \}

    struct in_place_t { };
    constexpr in_place_t in_place { };

    //! Wrap any single-threaded datastructure with Left-Right
    //! concurrency control
    //!
    //! Left-Right concurrency allows wait-free population-oblivious reads
    //! and blocking writes. Writers never block readers.
    //!
    //! Instances of this class maintain two full copies of the underlying
    //! datastructure and all modifications are performed twice. Consequently,
    //! uses of this class should be limited to small amounts of data where
    //! the number of reads dominates the number of writes.
    //!
    //! Left-Right concurrency control is described in depth in
    //!   A. Correia and P. Ramalhete. Left-Right: A Concurrency Control
    //!   Technique with Wait-Free Population Oblivious Reads
    template <typename T, typename ReaderRegistry>
    class basic_leftright
    {
        static_assert(noexcept(std::declval<ReaderRegistry>().arrive()),
                "ReaderRegistry::arrive() must be noexcept");
        static_assert(noexcept(std::declval<ReaderRegistry>().depart()),
                "ReaderRegistry::depart() must be noexcept");
        static_assert(noexcept(std::declval<const ReaderRegistry>().empty()),
                "ReaderRegistry::empty() must be noexcept");

      public:
        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;

        basic_leftright() = default;

        //! Construct the two underlying instances of T
        //! by moving a seed instance
        basic_leftright(T&& seed)
            noexcept(std::is_nothrow_copy_constructible<T>::value
                    && std::is_nothrow_move_constructible<T>::value);


        //! Construct the two underlying instances of T
        //! by copying a seed instance
        basic_leftright(const value_type& seed)
            noexcept(std::is_nothrow_copy_constructible<T>::value);


        //! Construct the two underlying instances of T in place, forwarding
        //! the args after the mpm::in_place tag
        template <typename... Args>
        basic_leftright(in_place_t, Args&&...)
            noexcept(std::is_nothrow_copy_constructible<T>::value
                    && std::is_nothrow_constructible<T, Args...>::value);

        //! \internal
        //!  Need a use-case for these. It seems that you would never want to
        //!  move/copy/swap the full leftright instance but rather apply those
        //!  operations to the encapsulated instance, in which case the relevant
        //!  operation is accessed via modify()

        basic_leftright(const basic_leftright& other)=delete;
        basic_leftright(basic_leftright&& other)=delete;
        basic_leftright& operator=(const basic_leftright& rhs)=delete;
        basic_leftright& operator=(basic_leftright&& rhs)=delete;
        void swap(basic_leftright& other) noexcept = delete;


        //! Modify the state of the managed datastructure
        //!
        //! Blocks/is-blocked-by other concurrent writers; does not
        //! block concurrent readers
        //!
        //! This function requires that execution of the supplied functor
        //! be noexcept.
        //!
        //! The function passed will be executed twice and *must*
        //! result in the exact same mutation operation being applied
        //! in both cases. For example it would be incorrect to supply
        //! a function here that inserted a random number into the
        //! underlying datastructure if said random number were calculated
        //! for each invocation (i.e. each invocation would insert a
        //! different value).
        //!
        //! \throws std::system_error on failure to lock internal mutex
        //!
        //! \internal I wanted the declaration to be as below so that this
        //!           function doesn't even exist for non-noexcept functors,
        //!           but g++ has not yet implemented noexcept mangling
        //!           as of version 5.2.1. Instead we just static_assert the
        //!           noexcept-ness of the functor in the body of the function
        //!
        //! template <typename F>
        //! auto modify(F f)
        //!     -> typename std::enable_if<noexcept(f(std::declval<T&>())),
        //!             typename std::result_of<F(T&)>::type>::type;
        template <typename F>
        typename std::result_of<F(T&)>::type modify(F f);

        //! Observe the state of the managed datastructure
        //!
        //! Always wait-free provided ReaderRegistry::arrive() and
        //! ReaderRegistry::depart() are truly wait-free
        //!
        //! \throws Whatever the provided functor throws and nothing else
        template <typename F>
        typename std::result_of<F(const T&)>::type observe(F f) const
            noexcept(noexcept(f(std::declval<const T&>())));

      private:
        class scoped_read_indication
        {
          public:
            scoped_read_indication(ReaderRegistry& rr) noexcept;
            ~scoped_read_indication() noexcept;
          private:
            ReaderRegistry& m_reg;
        };

        template <typename Lock>
        void toggle_reader_registry(Lock& l) noexcept;

        enum lr { read_left, read_right };

        mutable std::array<ReaderRegistry, 2> m_reader_registries;

        std::atomic_size_t m_registry_index { 0 };

        std::atomic<lr> m_leftright { read_left };

        T m_left alignas(MPM_LEFTRIGHT_CACHE_LINE_SIZE);
        T m_right alignas(MPM_LEFTRIGHT_CACHE_LINE_SIZE);
        std::mutex m_writemutex;
    };


    //! Simple implementation of ReaderRegistry concept
    //!
    //! This implemtation is wait-free but readers will contend
    //! on a single cache line due to the use of a shared counter.
    //!
    //! \concept{ReaderRegistry}
    class alignas(MPM_LEFTRIGHT_CACHE_LINE_SIZE) atomic_reader_registry
    {
      public:
        void arrive() noexcept;
        void depart() noexcept;
        bool empty() const noexcept;

      private:
        std::atomic_uint_fast32_t m_count{0};
    };


    //! Distributed implementation of ReaderRegistry
    //!
    //! Uses an array of N counters (suitably padded) and hashes reader
    //! thread ids to indices into said array so that concurrent reader
    //! registration can be made unlikely to contend. The likelihood
    //! of a collision is dependent on the number of concurrent readers
    //! relative to N.
    //!
    //! arrive() and depart() will perform better if N is a power of two.
    //!
    //! \concept{ReaderRegistry}
    template <std::size_t N, typename Hasher=std::hash<std::thread::id>>
    class alignas(MPM_LEFTRIGHT_CACHE_LINE_SIZE) distributed_atomic_reader_registry
    {
      public:
        void arrive() noexcept;
        void depart() noexcept;
        bool empty() const noexcept;

      private:
        class alignas(MPM_LEFTRIGHT_CACHE_LINE_SIZE) counter
        {
          public:
            void incr() noexcept;
            void decr() noexcept;
            std::uint_fast32_t relaxed_read() const noexcept;
          private:
            std::atomic_uint_fast32_t m_value{0};
        };

        std::array<counter, N> m_counters{};
    };


    //! Default leftright uses the simpler reader registry; prefer
    //! distributed_atomic_reader_registry when reads are highly contended
    template <typename T>
    using leftright = basic_leftright<T, atomic_reader_registry>;


#   ifndef DOCS

    template <typename T, typename R>
    basic_leftright<T, R>::basic_leftright(T&& seed)
            noexcept(std::is_nothrow_copy_constructible<T>::value
                    && std::is_nothrow_move_constructible<T>::value)
        : m_left(std::move(seed))
        , m_right(m_left)
    {
    }


    template <typename T, typename R>
    basic_leftright<T, R>::basic_leftright(const T& seed)
        noexcept(std::is_nothrow_copy_constructible<T>::value)
        : m_left(seed)
        , m_right(seed)
    {
    }


    template <typename T, typename R>
    template <typename... Args>
    basic_leftright<T, R>::basic_leftright(in_place_t, Args&&... args)
            noexcept(std::is_nothrow_copy_constructible<T>::value
                    && std::is_nothrow_constructible<T, Args...>::value)
        : m_left(std::forward<Args>(args)...)
        , m_right(m_left)
    {
    }


    template <typename T, typename R>
    template <typename F>
    typename std::result_of<F(const T&)>::type
    basic_leftright<T, R>::observe(F f) const
        noexcept(noexcept(f(std::declval<const T&>())))
    {
        std::size_t idx = m_registry_index.load(std::memory_order_acquire);
        scoped_read_indication sri(m_reader_registries[idx]);
        return read_left == m_leftright.load(std::memory_order_acquire)
            ? f(m_left)
            : f(m_right);
    }


    template <typename T, typename R>
    template <typename F>
    typename std::result_of<F(T&)>::type
    basic_leftright<T, R>::modify(F f)
    {
        static_assert(noexcept(f(std::declval<T&>())), "Modify functor must be noexcept");
        std::unique_lock<std::mutex> xlock(m_writemutex);
        if(read_left == m_leftright.load(std::memory_order_relaxed))
        {
            f(m_right);
            m_leftright.store(read_right, std::memory_order_release);
            toggle_reader_registry(xlock);
            return f(m_left);
        }
        else
        {
            f(m_left);
            m_leftright.store(read_left, std::memory_order_release);
            toggle_reader_registry(xlock);
            return f(m_right);
        }
    }


    template <typename T, typename R>
    template <typename Lock>
    void
    basic_leftright<T, R>::toggle_reader_registry(Lock& l) noexcept
    {
        assert(l);
        const std::size_t current =
            m_registry_index.load(std::memory_order_acquire);
        const std::size_t next = (current + 1) & 0x1;

        while(!m_reader_registries[next].empty())
        {
            std::this_thread::yield();
        }

        m_registry_index.store(next, std::memory_order_release);

        while(!m_reader_registries[current].empty())
        {
            std::this_thread::yield();
        }
    }


    template <typename T, typename R>
    basic_leftright<T, R>::scoped_read_indication::scoped_read_indication(R& r) noexcept
        : m_reg(r)
    {
        m_reg.arrive();
    }


    template <typename T, typename R>
    basic_leftright<T, R>::scoped_read_indication::~scoped_read_indication() noexcept
    {
        m_reg.depart();
    }


    void
    atomic_reader_registry::arrive() noexcept
    {
        m_count.fetch_add(1, std::memory_order_release);
    }


    void
    atomic_reader_registry::depart() noexcept
    {
        m_count.fetch_sub(1, std::memory_order_release);
    }


    bool
    atomic_reader_registry::empty() const noexcept
    {
        return 0 == m_count.load(std::memory_order_acquire);
    }


    template <std::size_t N, typename Hasher>
    void
    distributed_atomic_reader_registry<N, Hasher>::arrive() noexcept
    {
        std::size_t index = Hasher()(std::this_thread::get_id()) % N;
        m_counters[index].incr();
    }


    template <std::size_t N, typename Hasher>
    void
    distributed_atomic_reader_registry<N, Hasher>::depart() noexcept
    {
        std::size_t index = Hasher()(std::this_thread::get_id()) % N;
        m_counters[index].decr();
    }


    template <std::size_t N, typename Hasher>
    bool
    distributed_atomic_reader_registry<N, Hasher>::empty() const noexcept
    {
        bool retval = std::none_of(begin(m_counters), end(m_counters),
                [](const counter& ctr) { return ctr.relaxed_read(); });
        std::atomic_thread_fence(std::memory_order_acquire);
        return retval;
    }


    template <typename std::size_t N, typename H>
    void
    distributed_atomic_reader_registry<N, H>::counter::incr() noexcept
    {
        (void) m_value.fetch_add(1, std::memory_order_release);
    }


    template <typename std::size_t N, typename H>
    void
    distributed_atomic_reader_registry<N, H>::counter::decr() noexcept
    {
        (void) m_value.fetch_sub(1, std::memory_order_release);
    }


    template <typename std::size_t N, typename H>
    std::uint_fast32_t
    distributed_atomic_reader_registry<N, H>::counter::relaxed_read() const noexcept
    {
        return m_value.load(std::memory_order_relaxed);
    }

#   endif

}
