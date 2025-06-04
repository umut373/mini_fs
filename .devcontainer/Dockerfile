FROM ubuntu:22.04

RUN apt update
RUN apt install openssh-server sudo -y
RUN useradd -rm -d /home/ubuntu -s /bin/bash -g root -G sudo -u 1000 code
RUN usermod -aG sudo code
RUN echo 'code:1234' | chpasswd

RUN apt update
RUN apt install build-essential -y
RUN apt install gdb -y

RUN apt update
RUN apt install valgrind -y

RUN apt update
RUN apt install git -y

CMD ["/usr/bin/bash"]
