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
    file \
    flex \
    bison \
    curl \
    gcc-multilib \
    g++-multilib \
    tmux \
    vim \
  && apt autoremove -y && apt clean -y \
  && rm -rf /var/lib/apt/lists/* \
  # Install GCC
  && chmod +x /reify/install/install-gcc.sh \
  && /reify/install/install-gcc.sh \
  # Install Bitwuzla
  && chmod +x /reify/install/install-bitwuzla.sh \
  && /reify/install/install-bitwuzla.sh \
  # Cleanup
  && rm -rf /reify/.git \
  && rm -rf /reify/.gitignore \
  && rm -rf /reify/.vscode

ENTRYPOINT ["bash"]
