#!/bin/bash

# Open a netcat connection to the server
exec 3<>/dev/tcp/localhost/4000

# Send the request in chunks
echo -n "GET / HTTP/1.1" >&3
echo -n $'\r\n' >&3
echo -n "Host: localhost" >&3
echo -n $'\r\n' >&3
echo -n $'\r\n' >&3

# Read the response
cat <&3

# Close the connection
exec 3>&-
exec 3<&-