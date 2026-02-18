FROM ubuntu:noble

# Install sudo
RUN apt update \
    && apt install -y sudo \
    && rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# Add ubuntu user to the sudoers file
RUN echo ubuntu ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/ubuntu \
    && chmod 0440 /etc/sudoers.d/ubuntu

# Install dependencies for riscv-gnu-toolchain
RUN apt update \
    && apt install -y \
        autoconf \
        automake \
        autotools-dev \
        bc \
        bison \
        build-essential \
        cmake \
        curl \
        flex \
        gawk \
        git \
        gperf \
        libexpat-dev \
        libglib2.0-dev \
        libgmp-dev \
        libmpc-dev \
        libmpfr-dev \
        libslirp-dev \
        libtool \
        ninja-build \
        patchutils \
        python3 \
        python3-pip \
        python3-tomli \
        texinfo \
        zlib1g-dev \
    && rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# Install riscv-gnu-toolchain from source
RUN git clone https://github.com/riscv/riscv-gnu-toolchain \
    && cd riscv-gnu-toolchain \
    && ./configure --prefix=/opt/riscv --with-arch=rv32im --with-abi=ilp32 --enable-llvm --disable-linux \
    && make -j$(nproc) all \
    && cd .. \
    && rm -rf riscv-gnu-toolchain

# Install dependencies for Lavinium
RUN apt update \
    && apt install -y \
        binutils \
        bzip2 \
        coreutils \
        findutils \
        grep \
        gzip \
        libboost-all-dev \
        libbz2-dev \
        liblpsolve55-dev \
        libssl-dev \
        libxml2-dev \
        lp-solve \
        make \
        sed \
        sharutils \
        tar \
        unzip \
        xz-utils \
        zip \
    && rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# Link the lpsolve library to make it available for compilation
RUN ln -sf /usr/lib/lp_solve/liblpsolve55.so /usr/lib/liblpsolve55.so

# Install pandas to make Lavinium test.py and test_mt.py work
RUN apt update \
    && apt install -y \
        python3-pandas \
    && rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# Install dependencies for llvm installer

RUN apt update \
    && apt install -y \
        gnupg \
        lsb-release \
        software-properties-common \
        wget \
    && rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# Install llvm
RUN wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh all \
    && rm -rf /var/cache/apt/archives /var/lib/apt/lists/*

# Add llvm to PATH
ENV PATH="/usr/lib/llvm-20/bin:$PATH"

USER ubuntu
