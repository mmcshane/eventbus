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
wait-free _within the event bus_. It may be the case that event _handlers_ are not
wait-free.

Event subscriptions are aware of event-type polymorphism. That is to say that
given a pair of event types called Base and Derived where Base is a base class
of Derived, a subscription to Base event types will be invoked when an event of
type Derived is published. This is accomplished with a slight augmentation to
normal C++ inheritance -- Derived must extend Base via the
mpm::polymorphic_event class rather than directly. Rather than the following
"normal" inheritance:

    struct Derived : Base {}

Derived should be defined as

    struct Derived : mpm::polymorphic_event<Derived, Base> {}

Defining Derived in this manner will allow and event of this type to be
delivered as both Derived and Base.

It is not required that mpm::polymorphic_event be used. Any instance of a class
type can be published, however if polymorphic delivery is desired then
mpm::polymorphic_event must be used.

## Example Usage

Let's define some events

    struct my_base_event : mpm::polymorphic_event<my_base_event>
    {
        int x = 12;
    };

    struct my_derived_event : mpm::polymorphic_event<my_derived_event, my_base_event>
    {
        int y = 5;
    };

    struct my_object {};

    struct my_non_polymorphic_event : my_object
    {
        int foo = -1;
    };

Here's a quick look at publishing and subscribing to a polymorphic event

    mpm::eventbus ebus;

    // two subscriptions - 1 for my_base_event and 1 for my_derived_event
    auto base_subscription_cookie = ebus.subscribe<my_base_event>(
        [](const my_base_event& mbe) noexcept { std::cout << "handling a base event" << mbe.x; }
    );
    auto derived_subscription_cookie = ebus.subscribe<my_derived_event>(
        [](const my_derived_event& mde) noexcept { std::cout << "handling a derived event" << mde.y; }
    );
    
    // publish
    ebus.publish(my_derived_event{});

    // unsubscribe
    ebus.unsubscribe(base_subscription_cookie);
    ebus.unsubscribe(derived_subscription_cookie);
    
Some things worth noting here
* Both event handlers will fire
* It's generally preferable to use the provided mpm::scoped_subscription\<EventType>
  RAII container to handle the unsubscribe call automatically. I wanted to show
  unsubscription explicitly for this example.

For non-polymorphic dispatch, any object type can be published and it will be
handled by handlers for that exact type.

    mpm::eventbus ebus;

    // two subscriptions - 1 for my_object, 1 for my_non_polymorphic_event
    auto object_subscription_cookie = ebus.subscribe<my_object>(
        [](const my_object& mo) noexcept { std::cout << "handling a my_object"; }
    );
    auto non_poly_subscription_cookie = ebus.subscribe<my_non_polymorphic_event>(
        [](const my_non_polymorphic_event& mnpe) noexcept {
            std::cout << "handling a my_non_polymorphic_event " << mnpe.foo;
        }
    );

    // publish
    ebus.publish(my_non_polymorphic_event{});

    // unsubscribe
    ebus.unsubscribe(object_subscription_cookie);
    ebus.unsubscribe(non_poly_subscription_cookie);

Note with the above example that _only_ the handler for my_non_polymorphic_event
will fire because the inheritance relationship was not established via
mpm::polymorphic_event.

## Building

There's really no build needed as this is a header-only library, however if you
want to run the unit tests or generate docs you can use the cmake build. To
perform an out-of-tree build

    $ cd /tmp # or wherever
    $ mkdir eventbus-debug && cd $_
    $ cmake -DCMAKE_BUILD_TYPE=debug /path/to/eventbus/repository
    $ make && make test
    $ make docs # generates doxygen documentation under /tmp/eventbus-debug/docs

## TODO
- As it stands, publishing events from within event handlers is allowed. It's
  not clear that this is good.
- Implement an asynchronous publication proxy
- Maybe allow non-noexcept subscribers but unsubscribe them if they throw?
- Document Event and EventHandler concepts
- Pull leftright as an external project rather than embedding it under
  thirdparty/
