_This project has been created as part of the 42 curriculum by fducrot, lgranger, ludebarn._

Description
-----------

Webserv is an HTTP/1.1 server written in C++98, without any external library.
The goal is to understand how a web server works by building one: sockets,
non blocking I/O, request parsing, response generation and CGI execution.

The server uses a single poll loop for every socket and pipe, so one slow
client never blocks the others. Its behavior is driven by a configuration
file inspired by NGINX.

Key concepts:

- Socket: an endpoint for network communication, seen by the program as a
  file descriptor. The server opens one listening socket per address and
  port, then gets one new socket for each client it accepts.
- Non blocking I/O with poll: instead of waiting on one client, the server
  asks poll which sockets and pipes are ready, and only reads or writes on
  those. This is how one process serves many clients at the same time.
- CGI (Common Gateway Interface): a standard way to let an external program
  build the response. For a file like hello.py, the server starts the
  interpreter in a child process, passes the request data through
  environment variables and the body on stdin, then reads the headers and
  the page from its stdout.
- WebSocket: a different protocol that keeps a two way channel open between
  a browser and a server. It starts as an HTTP request but is not part of
  this project. Webserv only speaks HTTP/1.1, one request and one response
  at a time.

Main features:

- GET, POST and DELETE methods
- Static website, index files and directory listing (autoindex)
- Default and custom error pages
- File upload with a configurable storage directory
- Request body size limit and chunked request bodies
- HTTP redirections (return directive)
- Several servers, ports and virtual hosts (server_name)
- CGI execution based on the file extension

- Log file with timestamped server events

Bonus
-----

Cookies and sessions: the server parses the Cookie header of each request,
passes it to CGI scripts as HTTP_COOKIE, and forwards every Set-Cookie header
sent back by a CGI. The example www/cgi-bin/cookie.py gives a new client a
sessionid and counts its visits. Two clients get two distinct sessions.

    curl -i -c jar -b jar http://localhost:8080/cgi-bin/cookie.py

Run it twice: the first answer sets the cookies, the second one shows the
visit counter increasing.

The server also handles sessions itself, without any CGI, on the /session
route. On the first visit it creates a random sessionid (16 bytes read from
/dev/urandom), keeps it in memory and sends it back in a Set-Cookie header
(Path=/, HttpOnly). On each next visit it finds the session from the cookie and
increases its visit counter. An unknown or fake sessionid gets a new session.

    curl -i -c jar -b jar http://localhost:8080/session

The first answer carries Set-Cookie and visits 1, the second one has no
Set-Cookie and shows visits 2 with the same id. Sessions are lost when the
server stops.

Multiple CGI types: one location can map several extensions to several
interpreters, for example .py to python3 and .php to php-cgi.

    curl -i http://localhost:8080/cgi-bin/hello.py
    curl -i http://localhost:8080/cgi-bin/hello.php

Logger
------

At startup, the output of the server is redirected to ./webserv.log, so the
terminal stays clean. Each line is timestamped with a level, for example
info or error. It records the startup, the listening addresses, accepted and
closed connections, CGI starts, the shutdown, and system call failures such
as poll or accept. If the file cannot be opened, the server keeps logging to
the terminal.

    11/09/2026 08:42:21 [info] accept 127.0.0.1:55348 on fd 7
    11/09/2026 08:42:21 [info] GET /cgi-bin/cookie.py -> cgi started
    11/09/2026 08:42:21 [info] close 127.0.0.1 on fd 7

Follow the log while the server runs:

    tail -f webserv.log

Instructions
------------

Requirements: a C++ compiler (clang++ or c++), make, python3 and php-cgi
for the CGI examples.

Build:

    make

Other rules: make clean, make fclean, make re.

Run with a configuration file:

    ./webserv conf/default.conf

Then open http://localhost:8080 in a browser, or use curl:

    curl -i http://localhost:8080/
    curl -i -X POST --data-binary @file.txt http://localhost:8080/uploads/file.txt
    curl -i -X DELETE http://localhost:8080/uploads/file.txt
Configuration: each server block accepts listen, server_name,
client_max_body_size, error_page and location blocks. A location accepts
root, index, autoindex, allow_methods, return, upload_store, cgi_ext and
cgi_pass. Commented examples are in conf/default.conf, and invalid files used
for tests are in conf/bad.

Tests: shell and Python scripts are in the tests directory. The 42 tester
runs with conf/tester.conf.

Resources
---------

- RFC 9110, HTTP Semantics
- RFC 9112, HTTP/1.1
- RFC 3875, The Common Gateway Interface
- RFC 6265, HTTP State Management (cookies)
- NGINX documentation, nginx.org/en/docs
- Beej Guide to Network Programming
- MDN Web Docs, HTTP section
- Linux manual pages: socket, poll, fork, execve, pipe

AI usage: an AI assistant (Claude) was used as a support tool for:

- debugging, mainly the performance issues found with the 42 tester
  (event loop stalls on large CGI uploads)
- writing and extending test scripts
- generating the Doxygen configuration and reviewing documentation
- drafting this README

All code produced with AI help was read, tested and understood by the team
before being merged.
