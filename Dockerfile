# =========================
# Stage 1: Build
# =========================
FROM ubuntu:24.04 AS builder

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        libboost-all-dev \
        libasio-dev \
        nlohmann-json3-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B build && \
    cmake --build build -j$(nproc)


# =========================
# Stage 2: Production
# =========================
FROM ubuntu:24.04 AS runtime

RUN apt-get update && \
    apt-get install -y \
        libboost-system1.83.0 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/network-monitor ./network-monitor

EXPOSE 9000

CMD ["./network-monitor"]
