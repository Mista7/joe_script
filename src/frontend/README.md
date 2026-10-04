# JoeScript Studio

Run the local frontend from the repository root:

```sh
node src/frontend/server.mjs
```

Open <http://127.0.0.1:4173>. The frontend uses the existing `src/compiler` executable; build it first with `make -C src` if it is missing or out of date. The compiler and generated program run locally. Each run gets a temporary directory that is removed afterward.

The compiler currently emits `a.out`, which this frontend executes. Programs are limited to 5 seconds and 1 MB of captured output.