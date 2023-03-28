# Benchmarks

Here are some sample runs from the excellent [wrk](https://github.com/wg/wrk)
HTTP benchmarking tool. I ran these on a regular desktop computer running
Ubuntu LTS.

## Hello World

The example [Hello World](../examples/hello.cpp) web service.

- Single server thread
- Small JSON response `{"hello": "world"}`

```console
wrk --latency -c 128 -d 30s http://localhost:8080/hello
```

```console
Running 30s test @ http://localhost:8080/hello
  2 threads and 128 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   548.47us   26.67us   1.05ms   79.07%
    Req/Sec   115.91k     1.35k  118.29k    64.83%
  Latency Distribution
     50%  546.00us
     75%  562.00us
     90%  579.00us
     99%  630.00us
  6918410 requests in 30.00s, 587.21MB read
Requests/sec: 230608.15
Transfer/sec:     19.57MB
```

## Database

The example [Database](../examples/database.cpp) web service.

- Two server threads, one for HTTP and one for SQLite database queries
- Small JSON response `{"id":5249,"randomNumber":9529}`
- Query a random row from the `World` table

```console
wrk --latency -c 128 -d 30s http://localhost:8080/db
```

```console
Running 30s test @ http://localhost:8080/db
  2 threads and 128 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   655.41us   63.55us   1.32ms   86.21%
    Req/Sec    96.87k     3.30k  100.48k    79.83%
  Latency Distribution
     50%  649.00us
     75%  660.00us
     90%  700.00us
     99%    0.93ms
  5783676 requests in 30.00s, 561.39MB read
Requests/sec: 192787.75
Transfer/sec:     18.71MB
```
