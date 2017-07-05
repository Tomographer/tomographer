
Advanced Tomorun Compilation Options
====================================

It is possible to customize the tomorun executable by specializing it in various ways.
This is done by passing additional compilation flags to *cmake*.

Compilation flags should be passed in the *cmake* variable `TOMORUN_CXX_FLAGS`.  For
instance, you can specialize `tomorun` to only a single-qubit system with:

    > cmake .. -DTOMORUN_CXX_FLAGS="-DTOMORUN_CUSTOM_FIXED_DIM=2;-DTOMORUN_SUFFIX=_qubit"

You should always use a distinct and meaningful `TOMORUN_SUFFIX` when specializing your
executable, so that you can tell right away that the program is not the default-compiled
one.

See full documentation of the possible `tomorun` compile-time configuration options at:
https://tomographer.github.io/tomographer/api-doc/current/html/page_tomorun_config_build.html
