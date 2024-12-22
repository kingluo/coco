## Examples

### channel and waitgroup

Different coroutines form complex business logic through channels and waitgroups.

```bash
g++ -I. -O2 -Wall -Wextra -Werror examples/channel_and_waitgroup.cpp && ./a.out
---> FS WRITE, i=0
---> KAFKA produce message, i=0
---> FS WRITE, i=1
---> KAFKA produce message, i=1
---> FS WRITE, i=2
---> KAFKA produce message, i=2
---> fs_write_ch is closed.
---> kafka_produce_ch is closed.
---> ALL DONE! check errors if any.

```

### webserver

Transform the webserver [example](https://unixism.net/loti/tutorial/webserver_liburing.html) into coroutines.

```bash
$ g++ -I. -ggdb -O2 -Wall -Wextra -Werror examples/webserver.cpp -luring -o examples/webserver && (cd examples/ && ./webserver)
Minimum kernel version required is: 5.5
Your kernel version is: 6.8
ZeroHTTPd listening on port: 8000, fd=3
---> path=/
200 public/index.html 790 bytes
^CShutting down.

$ curl -i http://localhost:8000
HTTP/1.0 200 OK
Server: zerohttpd/0.1
Content-Type: text/html
content-length: 790

<!DOCTYPE html>
<html lang="en">

<head>
    <title>Welcome to ZeroHTTPd</title>
    <style>
        body {
            font-family: sans-serif;
            margin: 15px;
            text-align: center;
        }
    </style>
</head>

<body>
    <h1>It works! (kinda)</h1>
    <img src="tux.png" alt="Tux, the Linux mascot">
    <p>It is sure great to get out of that socket!</p>
    <p>ZeroHTTPd is a ridiculously simple (and incomplete) web server written for learning about Linux performance.</p>
    <p>Learn more about Linux performance at <a href="https://unixism.net/2019/04/28/linux-applications-performance-introduction/">unixism.net</a></p>
    <p>This version of ZeroHTTPd uses io_uring. Learn more about io_uring <a href="https://unixism.net/loti">here</a>.</p>
</body>
```
