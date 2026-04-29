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
    clang \
    lcov \
  && apt autoremove -y && apt clean -y \
  && rm -rf /var/lib/apt/lists/* \
  # Install LLVM-18
  && ln -s /usr/bin/opt-18 /usr/bin/opt \
  # Install GCC-12
  && chmod +x /reify/install/install-gcc.sh \
  && /reify/install/install-gcc.sh \
  # Install Bitwuzla
  && chmod +x /reify/install/install-bitwuzla.sh \
  && /reify/install/install-bitwuzla.sh \
  # Cleanup
  && rm -rf /reify/.git \
  && rm -rf /reify/.gitignore \
  && rm -rf /reify/.vscode \
  && rm -rf /reify/scripts/__pycache__

ENTRYPOINT ["bash"]
