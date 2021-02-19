# Release Notes

## Version 0.5.4

- The build requirements have been increased to require Clang >= 4.0, Xcode >= 9.0,
  and Python >= 3.6. Meanwhile, it is *recommended* to use GCC >= 7.4.0 and
  Boost >= 1.65.1. This effectively drops official support for Ubuntu 16.04 when
  using distribution-provided packages; ChronoSync may still work on this platform,
  but we provide no official support for it.

- Exceptions are now thrown using `NDN_THROW` instead of `BOOST_THROW_EXCEPTION`

- The private header files `config.hpp`, `common.hpp`, `bzip2-helper.hpp`,
  `mi-tag.hpp`, and `tlv.hpp` have been moved to a `detail` subdirectory

- Fix compilation against the latest version of ndn-cxx

- Fix incompatibility with the C++20 `<version>` header

- Upgrade `waf` to version 2.0.21

## Version 0.5.3

- Schedule sync Interest when receiving a NACK with reason `NoRoute` (Issue #5012)

- Use properly seeded pseudorandom number generator from ndn-cxx

- Fix compilation against the latest version of ndn-cxx

- Upgrade `waf` to version 2.0.14 and other build system improvements

## Version 0.5.2

- The build requirements have been upgraded to gcc >= 5.3 or clang >= 3.6.
  This effectively drops support for all versions of Ubuntu older
  than 16.04 that use distribution-provided compilers and packages.

- Transition to v0.3 packet format (Issues #4691 and #4684)

## Version 0.5.1

- Fix forceful shutdown of `Face` when destructing `Logic` instance (Issue #4525)

- Fix compilation against the latest version of ndn-cxx

- Upgrade `waf` to version 2.0.6 and other build system improvements

## Version 0.5.0

- *Breaking change:* Use bzip2 compression of sync Data payload (Issue #4140)

- Disallow Interest loopback on sync prefix (Issue #3979)

- Avoid ABI differences between debug/optimized modes (Issue #4496)

- Extend `Socket` and `Logic` API:

   - Allow customization of sync Interest lifetime (Issue #4490)

   - Limit the size of created sync Data and enable ability to
     customize the maximum packet size through environment variable
     (Issue #4140)

   - Allow override of the session number

- Disable use of Exclude filter (preparation for Exclude deprecation
  in NDN and implementation was only partially correct)
