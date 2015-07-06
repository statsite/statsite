FROM ubuntu
EXPOSE 8125 
RUN mkdir -p /statsite
RUN mkdir -p /var/run/statsite

ADD . /statsite

RUN apt-get update
RUN apt-get install -y build-essential check scons libjansson-dev libcurl4-openssl-dev libcurl3 libjansson4
RUN (cd statsite && make)

RUN apt-get purge -y build-essential check scons libcurl4-openssl-dev libjansson-dev
RUN apt-get autoremove -y

CMD ["/statsite/statsite", "-f", "/statsite/container/statsite.conf"]

