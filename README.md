# EventBus

It's an event bus. Subscribe to events. Publish events. Quoting the
[documentation for Google's implementation of the EventBus
concept](https://github.com/google/guava/wiki/EventBusExplained) in
[Guava](https://github.com/google/guava):

> EventBus allows publish-subscribe-style communication between components
> without requiring the components to explicitly register with one another (and
> thus be aware of each other). It is designed exclusively to replace
> traditional […] in-process event distribution using explicit registration. It
> is not a general-purpose publish-subscribe system, nor is it intended for
> interprocess communication.

Caution: there are downsides to every pattern or idiom. EventBuses are no
different. [Read about some of the down-sides and alternatives](http://endlesswhileloop.com/blog/2015/06/11/stop-using-event-buses/)
and really think about if this is an approach you want to adopt.

## Features

This library is fairly small and being header-only should be easy to integrate
into existing projects. A C++11 compiler is required. All operations are
threadsafe -- establishing and/or ending subscriptions concurrent with event
publication is safe for any number of threads. Moreover, event publication is
wait-free _within the event bus_. It may be the case that event _handlers_ are
not wait-free.

Event subscriptions are aware of event-type polymorphism. That is to say that
given a pair of event types called Base and Derived where Base is a base class
of Derived, a subscription to Base event types will be invoked when an event of
type Derived is published. This is accomplished with a slight augmentation to
normal C++ inheritance -- Derived must extend Base via the
mpm::enable_polymorphic_dispatch class rather than directly. Rather than the
following "normal" inheritance:

~~~{.cpp}
struct Derived : Base
{
}
~~~

Derived should be defined as

~~~{.cpp}
struct Derived : mpm::enable_polymorphic_dispatch<Derived, Base>
{
}
~~~

Defining Derived in this manner will allow and event of this type to be
delivered as both Derived and Base.

It is not required that mpm::enable_polymorphic_dispatch be used. Any instance
of a C++ object type can be published, however if polymorphic delivery is desired
then mpm::enable_polymorphic_dispatch must be used.

## Example Usage

Let's define some events

~~~{.cpp}
struct my_base_event : mpm::enable_polymorphic_dispatch<my_base_event>
{
    int x = 12;
};

struct my_derived_event
    : mpm::enable_polymorphic_dispatch<my_derived_event, my_base_event>
{
    int y = 5;
};

struct my_object {};

struct my_non_polymorphic_event : my_object
{
    int foo = -1;
};
~~~

Here's a quick look at publishing and subscribing to a polymorphic event

~~~{.cpp}
mpm::eventbus ebus;

// two subscriptions - 1 for my_base_event and 1 for my_derived_event

auto base_subscription = mpm::scoped_subscription<my_base_event> {
    ebus, [](const my_base_event& mbe) noexcept {
        std::cout << "handling a base event" << mbe.x;
    }
);

auto derived_subscription = mpm::scoped_subscription<my_derived_event> {
    ebus, [](const my_derived_event& mde) noexcept {
        std::cout << "handling a derived event" << mde.y;
    }
);

// publish
ebus.publish(my_derived_event{});

// subscriptions terminate at scope exit
~~~

Some things worth noting here
* Both event handlers will fire
* If you have a C++14 compiler, the callback can be declared with auto (e.g.
  const auto& event), removing the duplication in specifying the event type.

For non-polymorphic dispatch, any object type can be published and it will be
handled by handlers for _only_ that exact type.

~~~{.cpp}
mpm::eventbus ebus;

// two subscriptions - 1 for my_object, 1 for my_non_polymorphic_event

auto base_subscription = mpm::scoped_subscription<my_object> {
    ebus, [](const my_object& mo) noexcept {
        std::cout << "handling a my_object";
    }
};

auto non_poly_subscription = mpm::scoped_subscription<my_non_polymorhpic_event> {
    ebus, [](const my_non_polymorphic_event& mnpe) noexcept {
        std::cout << "handling a my_non_polymorphic_event " << mnpe.foo;
    }
);

// publish
ebus.publish(my_non_polymorphic_event{});

// subscriptions terminate at scope exit
~~~

Note with the above example that _only_ the handler for my_non_polymorphic_event
will fire because the inheritance relationship was not established via
mpm::enable_polymorphic_dispatch.

## Building

There's really no build needed as this is a header-only library, however if you
want to run the unit tests or generate docs you can use the cmake build. To
perform an out-of-tree build

~~~{.txt}
$ cd /tmp # or wherever
$ mkdir eventbus-debug && cd $_
$ cmake -DCMAKE_BUILD_TYPE=debug /path/to/eventbus/repository
$ make && make test
$ make docs # generates doxygen documentation under /tmp/eventbus-debug/docs
~~~

## TODO
- As it stands, publishing events from within event handlers is allowed. It's
  not clear that this is good.
- Implement an asynchronous publication proxy
- Maybe allow non-noexcept subscribers but unsubscribe them if they throw?
- Pull leftright as an external project rather than embedding it under
  thirdparty/
