# Parser Tests

This folder contains a small standalone test binary for `HttpRequestParser`.

## Run

```sh
make test
./tests/parser_tests
```

The test binary checks:
- valid `GET`
- valid `POST` with body
- `GET` with body rejected
- unsupported HTTP version
- missing `Content-Length` on `POST`
- body size larger than `client_max_body_size`
