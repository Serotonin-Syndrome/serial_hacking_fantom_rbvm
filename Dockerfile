FROM maven:3.6.0-jdk-10-slim

COPY ./web-ide /app/web-ide
COPY ./llvm-backend /app/llvm-backend
COPY ./vm /app/vm
COPY Makefile compile-and-run run-tests /app/
EXPOSE 80


RUN apt-get update && apt-get install -y clang-7 clang++-7 llvm-7 make git nano

WORKDIR /app/llvm-backend
RUN make CXX=clang++-7 -B

WORKDIR /app/vm
RUN make CXX=clang++-7 -B

WORKDIR /app/web-ide
RUN mvn clean install -DskipTests

WORKDIR /app
RUN mkdir web-ide/bin && cp llvm-backend/llvm-rbvm web-ide/bin/ && cp vm/vm web-ide/bin/rbvm && cp vm/da web-ide/bin/da

WORKDIR /app/web-ide
CMD java -jar target/ide.main-0.1.0.jar
