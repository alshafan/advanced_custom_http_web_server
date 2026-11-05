# Advanced – Custom HTTP Web Server

A responsive project website and a small static-file HTTP server written in C.

## Project structure
- `server.c` — TCP socket server and HTTP request/response handling
- `public/index.html` — project website
- `public/style.css` — responsive styling
- `public/app.js` — interactive HTTP response preview

## Requirements
- GCC or another C compiler
- Linux/macOS or another POSIX-compatible environment, or Windows with MinGW-w64

## Compile and run
Linux/macOS:
```bash
gcc -Wall -Wextra -O2 server.c -o server
./server 8080 public
```

Windows (MinGW-w64):
```powershell
gcc -Wall -Wextra -O2 server.c -o server.exe -lws2_32
.\server.exe 8080 public
```

Open **http://localhost:8080** in your browser. Stop with `Ctrl+C`.

You can change the port and document root:
```bash
./server 9000 public
```
On Windows, use `.\server.exe 9000 public`.

## Implemented
- TCP socket creation, bind, listen and accept
- HTTP GET and HEAD methods
- Basic request-line parsing
- Static file delivery
- MIME type selection for common extensions
- 200, 400, 403, 404, 405 and 414 responses
- Basic rejection of traversal-like paths

## Limitations / future enhancements
This is an educational starting point, not production-ready software. It is single-threaded, handles one request per connection, and does not implement TLS, persistent connections, chunked transfer encoding, complete URL decoding, authentication, directory listing or robust canonical-path validation. Before real deployment, add canonical path checks (e.g. `realpath` constrained to the document root), request timeouts, bounded header parsing, concurrency, logging, and TLS via a trusted library.
