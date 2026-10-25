# Docker container file for running Nedizblox Server
FROM alpine:latest AS builder

RUN apk add --no-cache \
    build-base \
    cmake \
    boost-dev \
    openssl-dev

WORKDIR /app

COPY . .

RUN mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DSERVER_BUILD=ON && make -j$(nproc)

FROM alpine:latest

RUN apk add --no-cache \
    libstdc++ \
    boost-system \
    boost-thread \
    openssl \
    ca-certificates

WORKDIR /app
COPY --from=builder /app/build/nedizblox-server /app/nedizblox-server

ENTRYPOINT [ "./nedizblox-server" ]

CMD [ "--port", "8080", "--map", "testplace.nbxl" ]