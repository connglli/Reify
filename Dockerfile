FROM ubuntu:24.04

COPY . /reify
WORKDIR /reify

RUN export DEBIAN_FRONTEND=noninteractive \
  && apt update \
  && apt install -y \
    build-essential \
    pkg-config \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    python3-venv \
    python3-mesonpy \
    m4 \
    libgmp10 \
    libgmp-dev \
    libmpfr6 \
    libmpfr-dev \
    tmux \
    vim \
  && apt autoremove -y && apt clean -y \
  && rm -rf /var/lib/apt/lists/* \
  && git clone --depth 1 https://github.com/bitwuzla/bitwuzla.git /tmp/bitwuzla \
  && cd /tmp/bitwuzla && ./configure.py && ninja -C build install && cd /reify \
  && rm -rf /tmp/bitwuzla

ENTRYPOINT ["bash"]
