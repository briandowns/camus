FROM alpine AS builder

RUN apk update && apk upgrade && \
    apk add --no-cache \
        ca-certificates \
        make \
        gcc \
        musl-dev \
        linux-headers

COPY . .
RUN make

FROM alpine
RUN apk update && apk upgrade && \
    apk add --no-cache \
        ca-certificates \
        make
COPY --from=builder bin/camus /usr/bin

ENTRYPOINT [ "camus" ]
