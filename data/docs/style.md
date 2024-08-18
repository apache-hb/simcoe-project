# Styleguide

## Checked exceptions

* use `throws(...)` defined in `core/throws.hpp` on functions to mark them as throwing
* only use the throws macro on the header definition, use `noexcept(false)` on source files
    * makes maintanence easier
* all functions should either be `noexcept` or `throws(...)`
* constructors and destructors must never throw
    * use named constructors instead of throwing ones to handle errors
    * throwing destructors can cause instant termination
* prefer `std::expected` and `TRY`/`TRY_UNWRAP`/`TRY_RESULT` for most cases
    * handling errors as values is *usually* more ergenomic
