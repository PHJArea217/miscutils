#!/bin/sh
set -eu
if [ "$1" = "" ]; then
	export NETNS_FILE=
else
	:>"$1"
	unshare -n mount --bind /proc/self/ns/net -- "$1"
	export NETNS_FILE="$1"
fi
unshare -i -m -p --fork --mount-proc --propagation=slave sh -eu -s "$@" <<\EOF
{
mount -t tmpfs -o mode=0755 none /proc/fs/nfsd
IDIR="$(mktemp -d "/proc/fs/nfsd/tmp.XXXXXXXXXX")"
case "$IDIR" in
	/proc/fs/nfsd/tmp.??????????)
		chmod 755 "$IDIR"
		if [ ! -f "$NETNS_FILE" ]; then
			NETNS_FILE="$IDIR/anon_netns"
			:>"$NETNS_FILE"
			unshare -n mount --bind /proc/self/ns/net "$NETNS_FILE"
		fi
		mkdir -p "$IDIR/conf" "$IDIR/etc/nginx" "$IDIR/etc/unbound" "$IDIR/host_etc"
		mount --bind -- "$2" "$IDIR/conf"
		shift 2
		cd "$IDIR"
		set -C
		echo nameserver 127.0.0.1 > etc/resolv.conf
		mount --rbind /etc host_etc
		mount --rbind conf/nginx etc/nginx
		mount --rbind conf/unbound etc/unbound
		printf "%s\n" > etc/inittab \
			"::respawn:/usr/bin/ld.so /usr/sbin/unbound" \
			"::respawn:$IDIR/conf/exec-nginx"
		for x in host_etc/* host_etc/.??*; do
			if [ -e "$x" ] && [ ! -e "${x#host_}" ]; then
				ln -s "$IDIR/$x" etc/ || :
			fi
		done
		if [ -d /run/systemd/resolve ]; then
			mount -t tmpfs -o mode=0755 none /run/systemd/resolve
		fi
		mount --rbind etc /etc
		export N_U_W_IDIR="$IDIR" N_U_W_NETNS="$NETNS_FILE"
		if [ -x conf/setup-netns-outer ]; then
			. conf/setup-netns-outer "$@"
		fi
		if [ -x conf/setup-netns-inner ]; then
			nsenter --net="$NETNS_FILE" conf/setup-netns-inner "$@"
		fi
		exec nsenter --net="$NETNS_FILE" /bin/busybox init
		;;
esac
exit 1
}
EOF
