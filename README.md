# cppchan

Multi-Producer, Multi-Consumer channel for multithreaded applications

## Description

This is a single header library that enables message passing between threads through a channel. This minimizes the necessity for basic synchronization primitives. `Channel` is a templated class that takes a variadic list of messages, which are just structures containing data. In order to initialize a channel you have to provide a handler for every message structure you passed into the template. For threads receiving and handling messages, you provide them a reference or pointer to the channel's receiver. For threads sending messages you pass off the transmitter.

## How to use

`/tests/tests.cpp` contains a basic implementation example on how to use Channel. 

1. Create the message structs you'd like to pass through the channel
2. Create the channels, passing in handlers for each message type in the same order that they are defined in the template arguments
3. Spawn the threads you want to spawn and have them use the channels to send or process the next message.