#!/bin/sh

set -euC
make_tmpdir() {
	TMPDIR_="${TMPDIR:-/tmp}/$(mktemp -u XXXXXXXXXX)"
	mkdir "$TMPDIR_" -m 700
	trap 'rm "$TMPDIR_"/*.tmp; rmdir "$TMPDIR_"' EXIT
}
static_routes() {
	(
	COUNTER=0
	while [ "$COUNTER" -lt 128 ]; do
		if [ "$COUNTER" -eq "$3" ]; then
			printf 'route add unreachable "%s:%x00:0:0:0:0/64"\n' "$1" "$COUNTER"
		else
			printf 'route add "%s:%x00:0:0:0:0/56" via "fe80::100:%x" dev "%s"\n' "$1" "$COUNTER" "$COUNTER" "$2"
		fi
		COUNTER="$((COUNTER+1))"
	done
	) > "$TMPDIR_/ipconf.tmp"
	ip -b "$TMPDIR_/ipconf.tmp"
}
# 1: prefix
# 2: device
# 3: index
setup_network_client() {
	make_tmpdir
	ip link add name "$2" type bridge
	sysctl -w net/ipv6/conf/"$2"/accept_ra=0
	printf 'a a "%s:ffff:310:0:0:%x/64" dev "%s"
a a "fe80::100:%x" dev "%s"
l set "%s" up\n
r a ::/0 via fe80::1 dev "%s"' "$1" "$3" "$2" "$3" "$2" "$2" "$2" > "$TMPDIR_/ipconf2.tmp"
	ip -b "$TMPDIR_/ipconf2.tmp"
	static_routes "$1" "$2" "$3"
	# Run unbound + dnsmasq
}
setup_network() {
	make_tmpdir
	ip link set lo up
	ip -6 route add unreachable "$1:0:0:0:0:0/48"
	ip -6 route add local "$1:8000:0:0:0:0/50" dev lo
	ip link add name "$2" type bridge
	sysctl -w net/ipv6/conf/"$2"/accept_ra=0
	ip a a fe80::1 dev "$2"
	ip a a "$1:ffff:300:0:0:1/64" dev "$2"
	ip l set dev "$2" up
	ip6tables -t mangle -A PREROUTING -i "$2" -d "$1:8000:0:0:0:0/51" -p tcp -j TPROXY --on-ip ::ffff:127.10.0.1 --on-port 1
	static_routes "$1" "$2" 256
	# Run haproxy
}
