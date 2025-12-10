###
# Do not edit the generated Dockerfile
###

# hadolint ignore=DL3006
FROM "${BASE_IMAGE}"

ENV DEBIAN_FRONTEND=noninteractive

ARG PG_VERSION

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# hadolint ignore=DL3008
RUN apt-get update && \
    apt-get -y install --no-install-recommends  \
      build-essential \
      ca-certificates \
      cdbs \
      curl \
      debhelper \
      debsigs \
      devscripts \
      dh-autoreconf \
      gettext \
      git \
      gnupg \
      lsb-release \
      make \
      rpm && \
    curl -o /etc/apt/keyrings/apt.postgresql.org.asc --fail https://www.postgresql.org/media/keys/ACCC4CF8.asc && \
    echo "deb [signed-by=/etc/apt/keyrings/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list && \
    apt-get update && \
    apt-get -y install --no-install-recommends postgresql-server-dev-all && \
    apt-get autoremove && \
    apt-get autoclean && \
    rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/bin/bash"]

CMD ["-i"]

LABEL org.opencontainers.image.source="${VCS_SOURCE}" \
      org.opencontainers.image.revision="${VCS_REVISION}" \
      org.opencontainers.image.vendor="OpenNMS Project" \
      org.opencontainers.image.authors="ronny@no42.org" \
      org.opencontainers.image.licenses="AGPLv3"

