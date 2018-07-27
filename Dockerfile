# This stage in the Dockerfile will not be included in the final image, as it
# is only being used to build the statsite binary.
FROM ubuntu:16.04 as builder

RUN apt-get update && apt-get install -y build-essential check libtool \
    automake autoconf gcc python python-requests scons pkg-config

ADD . /build
WORKDIR /build
RUN ./autogen.sh
RUN ./configure
RUN make
RUN make install
COPY statsite.docker.example /etc/statsite/statsite.conf
# At this point, we have built the binary and have installed all of the
# core files, which the following Dockerfile build stage will COPY in.

# ----------------------------------------------------------------------------

# This stage is what will be distributed. By pulling the compiled binary from
# the previous stage, we lessen image bloat and installed package set.
FROM ubuntu:16.04

RUN apt-get update && apt-get install -y nodejs python python-requests && rm -rf /var/lib/apt/lists/*
COPY --from=builder /etc/statsite/statsite.conf /etc/statsite/statsite.conf
COPY --from=builder /usr/local/bin/statsite /usr/local/bin/statsite
COPY --from=builder /usr/local/share/statsite/ /usr/local/share/statsite/

# You'll need to mount your configuration in here.
VOLUME /etc/statsite

ENTRYPOINT ["/usr/local/bin/statsite"]
CMD ["-f","/etc/statsite/statsite.conf"]
