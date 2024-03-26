# rpc-server

Remote Procedure Call (RPC) Server written in C.

<p>Author: Mitchell Elliott</p>
<p>Email: mitch.elliott@pacbell.net</p>

## Program Operation

### Compile:

```
make
make all: Build and compile everything
make clean: Remove object files
make spotless: Remove object files and the executable file
make format: Run .clang-format
```

### Run:

```
usage: rpcserver [hostname:port] -H size -N nthreads -I iterations -d dir
```

### Notes

<p>If the user does not specify the hostname or port number then the server will use the default hostname "localhost" and port 8912.</p>
<p>The default number of hash table buckets is 64 and the default number of threads is 4.</p>
<p>The default number of iterations for recursive lookup is 50.</p>
<p>The default directory for storing the log file is "data".</p>
<p> Once the server is running client programs can be run to connect with the server through a socket by using the same hostname and port number as the server. Multiple clients can interact with the server at the same time. The server will continue to process requests given to it by the clients until a fatal error occurs or the server is shut down.</p>
