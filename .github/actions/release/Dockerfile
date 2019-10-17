FROM alpine:latest

RUN	apk add --no-cache \
  bash \
  ca-certificates \
  curl \
  jq

COPY upload /usr/bin/upload

ENTRYPOINT ["/usr/bin/upload"]
