# -----------------------------------------------------------------------------
# Integration-test runner: standalone OpenUSD + the resolver built with memcached.
#
# Build context is the repository root:
#   docker compose -f tests/integration/compose.yml build runner
#
# Stages are ordered so the slow OpenUSD build is cached independently of the
# resolver sources: editing src/ only rebuilds the `resolver` stage.
# -----------------------------------------------------------------------------
ARG USD_VERSION=v25.05

FROM rockylinux:9 AS base
SHELL ["/bin/bash", "-eo", "pipefail", "-c"]

RUN dnf -y install epel-release dnf-plugins-core \
    && dnf config-manager --set-enabled crb \
    && dnf -y install \
        git gcc gcc-c++ cmake make \
        python3 python3-devel python3-pip \
        libmemcached-awesome-devel pkgconf-pkg-config \
        zip unzip which \
    && dnf clean all

ENV PIP_NO_CACHE_DIR=1
RUN python3 -m pip install --upgrade pip setuptools wheel \
    && python3 -m pip install ninja

# -----------------------------------------------------------------------------
FROM base AS usd
ARG USD_VERSION
ARG USD_JOBS=8

RUN git clone --depth 1 --branch "${USD_VERSION}" \
        https://github.com/PixarAnimationStudios/OpenUSD.git /tmp/OpenUSD \
    && python3 /tmp/OpenUSD/build_scripts/build_usd.py \
        --no-imaging --no-examples --no-tutorials --no-docs --no-tests \
        --no-tools --no-materialx --build-variant release -j "${USD_JOBS}" /opt/usd \
    && rm -rf /tmp/OpenUSD /opt/usd/build /opt/usd/src

# -----------------------------------------------------------------------------
FROM usd AS resolver
ARG RESOLVER_JOBS=8

WORKDIR /workspace
COPY . /workspace
# A Windows checkout (core.symlinks=false) turns git symlinks such as the bundled
# openssl3L/lib/libssl.so into text files holding the target name. Restore them.
RUN find /workspace/ext -type f -name '*.so*' -size -256c | while read -r f; do \
        target="$(head -c 255 "$f")"; \
        if [[ "$target" != */* && -f "$(dirname "$f")/$target" ]]; then ln -sfn "$target" "$f"; fi; \
    done
RUN python3 build_resolver.py --dcc usd --dcc-root /opt/usd --with-memcached \
        --build-type Release --jobs "${RESOLVER_JOBS}" --install-dir /opt/resolver \
    && rm -rf /workspace/build_usd_release

# -----------------------------------------------------------------------------
FROM usd AS runner
# Identifies the resolver build in published test records (lib/telemetry.py).
ARG RESOLVER_REF=local
ENV RIT_BUILD=${RESOLVER_REF}

COPY --from=resolver /opt/resolver /opt/resolver
COPY tests/integration/requirements.txt /tmp/requirements.txt
RUN python3 -m pip install -r /tmp/requirements.txt

ENV USD_ROOT=/opt/usd \
    PATH=/opt/usd/bin:${PATH} \
    PYTHONPATH=/opt/usd/lib/python:/opt/resolver/ayonUsdResolver/lib/python \
    LD_LIBRARY_PATH=/opt/usd/lib:/opt/resolver/ayonUsdResolver/lib \
    PXR_PLUGINPATH_NAME=/opt/resolver/ayonUsdResolver/resources \
    AYON_USD_RESOLVER_LOG_FILE_ENABLED=OFF

WORKDIR /tests
ENTRYPOINT ["python3", "-m", "pytest"]
CMD ["-v", "integration"]
