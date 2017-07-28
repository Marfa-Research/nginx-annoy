FROM ubuntu:16.10
MAINTAINER Joe Quadrino <joe@marfa.co>

RUN apt-get update && \
    apt-get install -y build-essential gcc git zip wget \
                       zlib1g-dev libpcre3 libpcre3-dev \
                       libev-dev libprotobuf-c-dev

ENV NGINX_VERSION 1.11.5

RUN wget -qO - http://nginx.org/download/nginx-$NGINX_VERSION.tar.gz | tar -xzf - -C /tmp

WORKDIR /tmp/nginx-$NGINX_VERSION
