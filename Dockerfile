FROM gcc
COPY . /app
WORKDIR /app
RUN make
EXPOSE 6339
CMD tc qdisc add dev eth0 root tbf rate 8kbit burst 8kbit latency 5ms && ./spider
