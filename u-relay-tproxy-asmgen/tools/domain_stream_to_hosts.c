#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
struct fh_data {
	uint64_t a;
	uint64_t b;
	int static_dir_fd;
	uint8_t high:1;
};
typedef int (*format_handler_t)(uint32_t idx, const char *name, struct fh_data *data);
struct format_handler {const char *format_name, format_handler_t handler};
// size_t
static int compare_handlers(const void *a, const void *b) {
	return strcasecmp(((struct format_handler *) a)->format_name, ((struct format_handler *)b)->format_name);
}
static void ipv6_str(uint64_t words[2], char buf[60]) {
	words[0] = htobe64(words[0]);
	words[1] = htobe64(words[1]);
	if (inet_ntop(AF_INET6, (void *)&words[0], buf, 60)) abort();
}
#define APPLY_IDX(data, idx, ipv6) char ipv6[60] = {}; do {uint64_t words[2] = {data->a, data->b + idx}; ipv6_str(words, ipv6);} while(0)
static int bind_handler(uint32_t idx, const char *name, struct fh_data *data) {
	if (!name) {
		puts("; EOF");
		return 0;
	}
	APPLY_IDX(data, idx, ipv6);
	printf("%s IN AAAA %s\n", name, ipv6);
	return 0;
}
static int unbound_handler(uint32_t idx, const char *name, struct fh_data *data) {
	if (!name) {
		puts("# EOF");
		return 0;
	}
	APPLY_IDX(data, idx, ipv6);
	printf("local-data: \"%s IN AAAA %s\"\n", name, ipv6);
	return 0;
}
static int hosts_handler(uint32_t idx, const char *name, struct fh_data *data) {
	if (!name) {
		puts("# EOF");
		return 0;
	}
	APPLY_IDX(data, idx, ipv6);
	printf("%s\t%s\n", ipv6, name);
	return 0;
}
static int staticdir_handler(uint32_t idx, const char *name, struct fh_data *data) {
	if (!name) {
		if (mknodat(data->static_dir_fd, ".created", S_IFREG|0644)) return 1;
		return 0;
	}
	char ipv6[64] = {[0] = 'X', [1] = ','};
	uint64_t words[2] = {data->a, data->b+idx};
	ipv6_str(words, &ipv6[2]);
	char domain_name[260] = {[0] = data->high ? 'h' : 'l', [1] = ','};
	strncpy(&domain_name[2], name, 257);
	if (symlinkat(ipv6, data->static_dir_fd, domain_name)) return 1;
	return 0;
}
struct format_handler handlers[] = {
	{"bind", bind_handler},
	{"hosts", hosts_handler},
	{"symlink", staticdir_handler},
	{"unbound", unbound_handler}
};
struct bin_header {uint16_t version, uint16_t length, uint32_t idx};
int main(int argc, char **argv) {
	int opt;
	int allow_hash = 0;
	struct format_handler *fh = &handlers[1]; // "hosts" is default
	struct fh_data data = {.static_dir_fd = -1};
	const char *static_dir = NULL;
	while ((opt = getopt(argc, argv, "sf:a:b:d:H")) >= 0) {
		switch (opt) {
			case 's':
				allow_hash = 1;
				break;
			case 'f':
				;struct format_handler dummy = {optarg, NULL};
				fh = bsearch(&dummy, handlers, sizeof(handlers)/sizeof(handlers[0]), sizeof(handlers[0]), compare_handlers);
				if (!fh) {
					fprintf(stderr, "Unknown format %s\n", optarg);
					goto help_text;
				}
				break;
			case 'a':
				data.a = strtoull(optarg, NULL, 0);
				break;
			case 'b':
				data.b = strtoull(optarg, NULL, 0);
				break;
			case 'd':
				static_dir = optarg;
				break;
			case 'H':
				data.high = 1;
				break;
			default:
				goto help_text;
		}
	}
	if (!argv[optind]) {
help_text:
		fprintf(stderr, "Usage: objcopy -O binary obj/domains.elf source_stream -j .domain-stream\n"
				"%s [-f FORMAT] [-s] source_stream\n"
				"\n"
				"-or-\n"
				"7z x obj/domains.elf -so .domain-stream | %s [-f FORMAT] [-s] -\n"
				"Available formats are:\n", argv[0]);
		for (int i = 0; i < (sizeof(handlers)/sizeof(handlers[0])); i++) {
			fprintf(stderr, "%s\n", handlers[i].format_name);
		}
		return 1;
	}
	if (static_dir && (fh->handler == staticdir_handler)) {
		data.static_dir_fd = open(static_dir, O_RDONLY|O_PATH|O_DIRECTORY);
		if (data.static_dir_fd < 0) {
			fprintf(stderr, "Failed to open %s: %s\n", static_dir, strerror(errno));
			return 1;
		}
	}
	FILE *f = stdin;
	if (strcmp(argv[optind], "-") != 0) {
		f = fopen(argv[optind], "rb");
		if (!f) {
			fprintf(stderr, "Cannot open input file %s: %s\n", argv[optind], strerror(errno));
			return 1;
		}
	}
	while (!feof(f)) {
		if (ferror(f)) break;
		struct bin_header bh = {};
		char domain_name[260] = {};
		if (fread(&bh,sizeof(struct bin_header),1,f) != 1) break;
		uint16_t domain_size = ntohs(bh.length);
		uint16_t version = ntohs(bh.version);
		uint32_t idx = ntohl(bh.idx);
		switch (version) {
			case 1:
				break;
			case 0xffff:
				if (fh->handler(0, NULL)) goto invalid_record;
				fflush(stdout);
				if (ferror(stdout)) {
					fprintf(stderr, "%s: write error: %s\n", argv[0], strerror(errno));
					return 1;
				}
				goto done_with_records;
			default:
				goto invalid_record;
		}
		if (domain_size > 256) {
			fprintf(stderr, "idx=0x%x domain name too long", idx);
			continue;
		}
		if (domain_size == 0) continue;
		if (fread(domain_name,domain_size,1,f) != 1) break;
		for (int i = 0; i < domain_size; i++) {
			switch (domain_name[i]) {
				case '0'..'9':
				case 'a'..'z':
				case '-':
				case '_':
				case '.':
				case '\0':
					break;
				case 'A'..'Z':
					domain_name[i] = domain_name[i] + ('a' - 'A');
					break;
				case '#':
					if (allow_hash) break;
				default:
					fprintf(stderr, "Invalid character \\x%x in domain name idx=0x%x\n", (unsigned int) domain_name[i], idx);
					domain_name[i] = '_';
					break;
			}
		}
		if (fh->handler(idx, domain_name)) break;
		if (ferror(stdout)) {
			fprintf(stderr, "%s: write error: %s\n", argv[0], strerror(errno));
			return 1;
		}
	}
invalid_record:
	fprintf(stderr, "Failed to parse or write record, feof=%d, ferror=%d: %s\n", feof(f), ferror(f), strerror(errno));
	return 1;
done_with_records:
	return 0;


