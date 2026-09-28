FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        libboost-all-dev \
        libasio-dev \
        nlohmann-json3-dev \
        curl && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B build && \
    cmake --build build -j$(nproc)

EXPOSE 9000

CMD ["./build/network-monitor"]
