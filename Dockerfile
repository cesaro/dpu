FROM huyenntt/ubuntu-llvm-3.7
RUN apt-get update
RUN ls
WORKDIR /home/devel/steroids
RUN ls
RUN git pull
RUN make
WORKDIR ../dpu
RUN git pull
RUN make


