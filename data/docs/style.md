# Styleguide

## Checked exceptions

* use `throws(...)` defined in `core/throws.hpp` on functions to mark them as throwing
* only use the throws macro on the header definition, use `noexcept(false)` on source files
    * makes maintanence easier
* all functions should either be `noexcept` or `throws(...)`
* constructors and destructors should never throw
    * use named constructors instead of throwing ones to handle errors
    * if a class is used as a scope guard and is never used in a container it can have throwing ctors/dtors
* prefer `std::expected` and `TRY`/`TRY_UNWRAP`/`TRY_RESULT` for most cases
    * handling errors as values is *usually* more ergenomic
