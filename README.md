### Запуск

1. Запуск `zookeeper`/`kafka`:
```
cd docker
docker-compose up --build
```
Далее:
```
docker exec kafka-node-1 kafka_2.13-3.0.0/bin/kafka-topics.sh --bootstrap-server localhost:9092 --create --topic event_server --replication-factor 1 --partitions 1
```
2. Запуск сервера:
```
mkdir CMakeFiles
cd CMakeFiles
cmake ..
make
./start_writer.sh
```
Ожидаемый вывод:
```
Consuming messages from topic event_server
Got assigned: [ event_server[0:#] ]
```
Далее в другом терминале:
```
./start.sh
```

### Проверка

Я проверял такими запросами:
```
(base) uta123@uta123-nix:~$ curl localhost:80/person -v --data-raw '{"login":"hilog","first_name":"ura","last_name":"ska","age":45}'
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 80 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 80 (#0)
> POST /person HTTP/1.1
> Host: localhost
> User-Agent: curl/7.58.0
> Accept: */*
> Content-Length: 63
> Content-Type: application/x-www-form-urlencoded
> 
* upload completely sent off: 63 out of 63 bytes
< HTTP/1.1 200 OK
< Date: Thu, 16 Dec 2021 16:30:55 GMT
< Connection: Keep-Alive
< Transfer-Encoding: chunked
< Content-Type: application/json
< 
* Connection #0 to host localhost left intact
{"queued":true}
(base) uta123@uta123-nix:~$ curl loca?login=hilog -X GET
{"age":45,"first_name":"ura","last_name":"ska","login":"hilog"}
```
