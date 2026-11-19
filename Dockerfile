# ==============================================================================
# Algo Dasturlash Tili - Universal Docker Konteyneri
# Linux, Windows (Docker Desktop), macOS (Apple Silicon / Intel)
# ==============================================================================
FROM alpine:3.19 AS builder

RUN apk add --no-cache gcc make musl-dev binutils

WORKDIR /build
COPY src/ src/
COPY std/ std/
COPY Makefile .

RUN make -s clean all

FROM alpine:3.19

RUN apk add --no-cache binutils

WORKDIR /app
COPY --from=builder /build/bin/algo /usr/local/bin/algo
COPY --from=builder /build/bin/algoc /usr/local/bin/algoc
COPY --from=builder /build/std /root/.algo/std

ENV PATH="/usr/local/bin:${PATH}"

ENTRYPOINT ["algo"]
CMD ["yordam"]
