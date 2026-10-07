# Building the released input source

The user-confirmed FreeFly-F3 DLL has SHA-256:

`9a6db746f29a069d452d60eeb16e399fa9e43a026781e445ac4c089af009826b`

Its source baseline is `e43be0d5c130b15980e07f6076fbca2250fbeece`, with the reviewed Map Maker, diagnostic and binary32 comparison patches applied in that order. `make` now prepares those sources automatically. The preparation verifies each changed translation unit against the exact compiled source hashes before writing; an unexpected layout aborts without overwriting the checkout. It is idempotent. It uses local Git, Python 3.10 or newer, and the existing MinGW toolchain; it does not access the network.

For source inspection without compiling:

```sh
python tools/prepare_freefly_f3.py
```

On a MinGW cross-build host:

```sh
make CC=i686-w64-mingw32-gcc WINDRES=i686-w64-mingw32-windres PYTHON=python3
```

The publication reuses the already tested DLL, not a rebuild with a new timestamp. The original compiler flags and native Windows regression results are preserved in the published source/verification archive. To edit the prepared source, deliberately revise or retire the pinned preparation step; it will not silently accept unreviewed changes.

Only Free Fly mouse look was interactively confirmed by the user. The existing native tests and source checks are not a claim that every menu or ROM has been gameplay-tested. No ROMs, user settings or save-state data belong in this repository.
