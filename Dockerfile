FROM ubuntu
EXPOSE 8125 
ADD . /statsite

RUN mkdir -p /statsite && mkdir -p /var/run/statsite && \
    apt-get update && \
    apt-get install -y build-essential check scons libjansson-dev libcurl4-openssl-dev libcurl3 libjansson4 && \
    (cd statsite && make) && \
    apt-get purge -y build-essential check scons libcurl4-openssl-dev libjansson-dev && \
    apt-get autoremove -y && apt-get clean && rm -rf /var/lib/apt/lists/*

VOLUME ["/etc/statsite"]
CMD ["/statsite/statsite", "-f", "/etc/statsite/statsite.conf"]

