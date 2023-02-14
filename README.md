# rw-json [![GitHub](https://img.shields.io/badge/License-MIT-blue?style=flat-square)](https://github.com/Rachel-Wirtz/rw-json/blob/latest/LICENSE) [![GitHub](https://img.shields.io/badge/Version-0.1.0-blue?style=flat-square)](https://github.com/Rachel-Wirtz/rw-json)

JSON in 2023, not only because of the growing number of web based applications, is one of the most common text based serialization formats used. Languages such as Python, C# (.NET), obviously Javascript and many others support it natively.

<br>

## What you get and what we keep working on

In its current version, rw-json provides already a fully working library for serialization and deserialization to and from JSON. However, we have some drawbacks to solve still.

* Currently values are copied into internal memory which clearly causes a memory overhead. To solve this we are working on an extension which allows binding of values to pointers which are then used instead for serialization. This is intended to work like `std::string_view` for example.
* The library provides template spezializations for `char8_t`, `char16_t` and `char32_t` character types, but doesn't set locales by itself. Therefor the stream based serialization in many cases might not work out of the box for character types other than `char` and `wchar_t`.

<br>

## How does it work?

rw-json is a single header file library. Even tho it is currently not longer than 1250 lines, the code is a bit messy to read by nature.
Therefor a quick start guide you find [here](https://github.com/Rachel-Wirtz/rw-json/wiki/Quick-Start) and a documentation explaining the entire public API for the current version is available as well [here](https://github.com/Rachel-Wirtz/rw-json/wiki/rw::json).

<br>

**Happy coding!**
