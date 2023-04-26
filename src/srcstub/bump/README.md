[![Build](https://github.com/Polynominal/bump.hpp/actions/workflows/cmake.yml/badge.svg)](https://github.com/Polynominal/bump.hpp/actions/workflows/cmake.yml)

# bump.hpp
C++ port of https://github.com/kikito/bump.lua

Background
============================

bump.hpp used to be a part of a bigger project, so bump itself is stored in the namespace **plugin::physics::bump** for now.

Additionally, two other utilites have been exported: **math::vec2** and **plugin::util::UserData**

You may need to adjust the namespaces if you wish to import it into your own project.

bump.hpp is built using CMake.


C++ related features
============================

 - Formal UserData class to handle arbitrary user data with optional memory management, provided you instantiate it with a shared_ptr
 - Collision responses are classes not string names. Helps with strong typing.
 - Object oriented structure
 - gtest tests, almost matching bump.lua

Examples
============================

See tests for usage examples.
