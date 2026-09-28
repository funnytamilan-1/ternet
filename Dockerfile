FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ cmake make ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release --parallel && ctest --test-dir build --output-on-failure -C Release

FROM ubuntu:24.04
RUN useradd --create-home --uid 10001 ternet
COPY --from=build /src/build/tnc /usr/local/bin/tnc
USER ternet
WORKDIR /workspace
ENTRYPOINT ["tnc"]
