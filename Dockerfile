FROM ubuntu

LABEL description="Proxy Docker"

RUN apt-get update && apt install -y g++ vim make curl net-tools python3.8 gdb

WORKDIR /code