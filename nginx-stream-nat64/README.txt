This nginx.conf file allows you to create a NAT64 with nginx.

Theory of operation

A traditional NAT64 functions as follows:
a server TCP socket bound to 64:ff9b::1 port 1, which redirects to 0.0.0.1 port 1.
socat TCP-LISTEN:1,bind=64:ff9b::1,fork,reuseaddr TCP:0.0.0.1:1

a server TCP socket bound to 64:ff9b::1 port 2, which redirects to 0.0.0.1 port 2.
socat TCP-LISTEN:2,bind=64:ff9b::1,fork,reuseaddr TCP:0.0.0.1:2

a server TCP socket bound to 64:ff9b::1 port 3, which redirects to 0.0.0.1 port 3.
socat TCP-LISTEN:3,bind=64:ff9b::1,fork,reuseaddr TCP:0.0.0.1:3

...

a server TCP socket bound to 64:ff9b::1 port 65534, which redirects to 0.0.0.1 port 65534.
socat TCP-LISTEN:65534,bind=64:ff9b::1,fork,reuseaddr TCP:0.0.0.1:65534

a server TCP socket bound to 64:ff9b::1 port 65535, which redirects to 0.0.0.1 port 65535.
socat TCP-LISTEN:65535,bind=64:ff9b::1,fork,reuseaddr TCP:0.0.0.1:65535

a server TCP socket bound to 64:ff9b::2 port 1, which redirects to 0.0.0.2 port 1.
socat TCP-LISTEN:1,bind=64:ff9b::2,fork,reuseaddr TCP:0.0.0.2:1

a server TCP socket bound to 64:ff9b::2 port 2, which redirects to 0.0.0.2 port 2.
socat TCP-LISTEN:2,bind=64:ff9b::2,fork,reuseaddr TCP:0.0.0.2:2

socat TCP-LISTEN:3,bind=64:ff9b::2,fork,reuseaddr TCP:0.0.0.2:3
socat TCP-LISTEN:4,bind=64:ff9b::2,fork,reuseaddr TCP:0.0.0.2:4
...
socat TCP-LISTEN:65534,bind=64:ff9b::ffff:fffe,fork,reuseaddr TCP:255.255.255.254:65534
socat TCP-LISTEN:65535,bind=64:ff9b::ffff:fffe,fork,reuseaddr TCP:255.255.255.254:65535

Except that we generate the TCP destination dynamically and don't have say, 280 trillion instances of socat.
