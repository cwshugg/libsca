# Lib SCA

This is a small C library written to assist with writing cache-based
side-channel attacks. So far, it provides functions that:

* Flush cache lines, given a virtual address.
* Times memory loads and stores, given a virtual address.
* Collects statistics on the system's cache hit/miss timing.
* Extract sections of bits from virtual addresses that correspond to cache line,
  cache set, and cache tag values.

The library code can be found in `lib/

I've also implemented a few tools and example programs that use the library.
These can be found in `tools/`.

## Building

To build the library, simply navigate into `lib/` and invoke the makefile.

```bash
cd lib
make
```

To build all tools within `tools/`, navigate into `tools/` and invoke the
makefile.

```bash
cd tools
make
```

