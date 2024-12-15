# Styleguide

## Checked exceptions

* use `throws(...)` defined in `core/throws.hpp` on functions to mark them as throwing
* only use the throws macro on the header definition, use `noexcept(false)` on source files
    * makes maintanence easier
* all functions should either be `noexcept` or `throws(...)`
* destructors must never throw
    * throwing destructors can cause instant termination

* prefer exceptions only for things that will happen spuriously
  * losing network connection should throw
  * a file not existing should not throw

* logging an error and continuing in a degraded state is a preferrable alternative to terminating
  * if a config file for something isnt found just return defaults

* data loss without notifying the user is *NEVER* acceptable
  * if something cannot be written, find an alternative path
  * if it *really* cant be written to disk, tell the user why and wait for them to resolve the problem
