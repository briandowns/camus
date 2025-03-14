FROM alpine AS builder

RUN apk update && apk upgrade && \
    apk add --no-cache \
        ca-certificates \
        make \
        gcc \
        musl-dev

COPY . .
RUN make

FROM scratch
COPY --from=builder bin/camus .
