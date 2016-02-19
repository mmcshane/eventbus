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

## TODO
- As it stands, publishing events from within event handlers is allowed. It's
  not clear that this is good.
- Implement an asynchronous publication proxy
- Maybe allow non-noexcept subscribers but unsubscribe them if they throw?
