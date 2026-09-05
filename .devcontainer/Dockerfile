FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-22.04

ARG CEPH_RELEASE=squid
ARG NODE_MAJOR=24

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y --no-install-recommends ca-certificates curl gnupg lsb-release \
    && mkdir -p /etc/apt/keyrings \
    && curl -fsSL https://download.ceph.com/keys/release.asc | gpg --dearmor -o /etc/apt/keyrings/ceph.gpg \
    && echo "deb [signed-by=/etc/apt/keyrings/ceph.gpg] https://download.ceph.com/debian-${CEPH_RELEASE}/ jammy main" > /etc/apt/sources.list.d/ceph.list \
    && curl -fsSL https://deb.nodesource.com/setup_${NODE_MAJOR}.x | bash - \
    && apt-get update && apt-get install -y --no-install-recommends \
       nodejs librados-dev librbd-dev ceph-common pkg-config \
    && rm -rf /var/lib/apt/lists/*
