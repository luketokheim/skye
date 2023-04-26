# Examples

Example apps for the Skye HTTP server framework. There is a
[live demo](https://skye-server.web.app/) running on Google Cloud Platform.

## Hello World

The example [Hello World](hello.cpp) web service.

- Single server thread
- Small JSON response `{"hello": "world"}`

```console
curl https://skye-server.web.app/hello
{"hello": "world"}
```

## Echo

- Single server thread
- Plain text response `Hello World`
- Transform input text based on the request path

```console
curl -d "Hello World" https://skye-server.web.app/echo
Hello World
```

```console
curl -d "Hello World" https://skye-server.web.app/lowercase
hello world
```

```console
curl -d "Hello World" https://skye-server.web.app/uppercase
HELLO WORLD
```

```console
curl -d "Hello World" https://skye-server.web.app/reverse
dlroW olleH
```

```console
curl -d "Hello World" https://skye-server.web.app/yell
Hello World!!
```

## Database

The example [Database](database.cpp) web service.

- Two server threads, one for HTTP and one for SQLite database queries
- Small JSON response `{"id":5249,"randomNumber":9529}`
- Query a random row from the `World` table

```console
curl https://skye-server.web.app/db
{"id":4412,"randomNumber":9262}
```

You can generate your own SQLite database file for testing with the
[sqlite3_schema.py](../tools/sqlite3_schema.py) script.

```console
python tools/sqlite3_schema.py | sqlite3 database.db
```
