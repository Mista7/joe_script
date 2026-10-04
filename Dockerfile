FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates clang g++ llvm-dev nodejs \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src/ /app/src/

RUN g++ $(llvm-config --cxxflags) -std=c++20 -O2 -Isrc \
      src/main.cc src/lexer.cc src/parser.cc src/semantics.cc \
      src/symbol-table.cc src/codegen.cc \
      $(llvm-config --ldflags --system-libs --libs all) \
      -o /usr/local/bin/joescript-compiler \
    && useradd --system --uid 10001 --create-home joescript

ENV COMPILER_PATH=/usr/local/bin/joescript-compiler
ENV HOST=0.0.0.0
ENV PORT=4173
ENV NODE_ENV=production

USER joescript
EXPOSE 4173
CMD ["node", "/app/src/frontend/server.mjs"]