// SPDX-License-Identifier: GPL-2.0
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"

#define REQ_BUF_SIZE 8192
#define HTML_MAX_SIZE (512 * 1024)

static volatile sig_atomic_t stop_server;

static void on_signal(int signo)
{
	(void)signo;
	stop_server = 1;
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [-c config_path] [-p port] [-w html_path]\n"
		"  -c path   Config file (default: /etc/cluster_os/config.conf)\n"
		"  -p port   Listen port (default: websocket.port from config)\n"
		"  -w path   Admin HTML path (default: tools/cluster_os/admin_server_control.html)\n",
		prog);
}

static void send_response(int fd, const char *status, const char *ctype,
			  const char *body, size_t body_len)
{
	char header[512];
	int n;
	size_t off;
	ssize_t wr;

	n = snprintf(header, sizeof(header),
		     "HTTP/1.1 %s\r\n"
		     "Content-Type: %s\r\n"
		     "Content-Length: %zu\r\n"
		     "Cache-Control: no-store\r\n"
		     "Connection: close\r\n\r\n",
		     status, ctype, body_len);
	if (n < 0)
		return;

	off = 0;
	while (off < (size_t)n) {
		wr = write(fd, header + off, (size_t)n - off);
		if (wr < 0) {
			if (errno == EINTR)
				continue;
			return;
		}
		off += (size_t)wr;
	}
	off = 0;
	while (body && off < body_len) {
		wr = write(fd, body + off, body_len - off);
		if (wr < 0) {
			if (errno == EINTR)
				continue;
			return;
		}
		off += (size_t)wr;
	}
}

static int load_html(const char *path, char **out, size_t *out_len)
{
	FILE *fp;
	long sz;
	size_t nread;
	char *buf;

	fp = fopen(path, "rb");
	if (!fp)
		return -errno;
	if (fseek(fp, 0, SEEK_END)) {
		fclose(fp);
		return -EIO;
	}
	sz = ftell(fp);
	if (sz < 0 || sz > HTML_MAX_SIZE) {
		fclose(fp);
		return -EFBIG;
	}
	if (fseek(fp, 0, SEEK_SET)) {
		fclose(fp);
		return -EIO;
	}
	buf = calloc((size_t)sz + 1, 1);
	if (!buf) {
		fclose(fp);
		return -ENOMEM;
	}
	nread = fread(buf, 1, (size_t)sz, fp);
	fclose(fp);
	if (nread != (size_t)sz) {
		free(buf);
		return -EIO;
	}

	*out = buf;
	*out_len = nread;
	return 0;
}

static void json_escape(const char *src, char *dst, size_t dst_len)
{
	size_t di = 0;
	size_t i;

	if (!dst_len)
		return;
	for (i = 0; src[i] && di + 2 < dst_len; i++) {
		unsigned char c = (unsigned char)src[i];

		if (c == '"' || c == '\\') {
			if (di + 2 >= dst_len)
				break;
			dst[di++] = '\\';
			dst[di++] = (char)c;
		} else if (c >= 0x20) {
			dst[di++] = (char)c;
		}
	}
	dst[di] = '\0';
}

static void send_state_json(int fd, const struct cluster_os_config *cfg)
{
	char node_id[CLUSTER_OS_MAX_STR * 2];
	char json[4096];
	int n;

	json_escape(cfg->general.node_id[0] ? cfg->general.node_id : "admin-node",
		    node_id, sizeof(node_id));
	n = snprintf(json, sizeof(json),
		     "{"
		     "\"clusters\":[\"prod-cluster\",\"dev-cluster\",\"test-cluster\",\"pod-cluster\"],"
		     "\"summary\":{\"up\":3,\"total\":3,\"workload\":\"orchestring\",\"status\":\"normal\"},"
		     "\"metrics\":{\"node\":\"prod-cluster-orchestring-1\",\"cpu\":45,\"memory\":60,\"disk\":30},"
		     "\"nodes\":["
		     "{\"name\":\"prod-cluster-orchestring-1\",\"online\":2,\"total\":3},"
		     "{\"name\":\"prod-cluster-orchestring-1\",\"online\":2,\"total\":3},"
		     "{\"name\":\"prod-cluster-orchestring-2\",\"online\":2,\"total\":3},"
		     "{\"name\":\"prod-cluster-orchestring-2\",\"online\":2,\"total\":3},"
		     "{\"name\":\"prod-cluster-orchestring-3\",\"online\":2,\"total\":3},"
		     "{\"name\":\"prod-cluster-orchestring-3\",\"online\":2,\"total\":3}"
		     "],"
		     "\"meta\":{\"node_id\":\"%s\",\"role\":\"%s\"}"
		     "}",
		     node_id, cluster_os_role_to_string(cfg->general.role));
	if (n < 0)
		return;
	send_response(fd, "200 OK", "application/json", json, (size_t)n);
}

static void handle_client(int cfd, const struct cluster_os_config *cfg,
			  const char *html, size_t html_len)
{
	char req[REQ_BUF_SIZE + 1];
	char method[16] = { 0 };
	char path[256] = { 0 };
	ssize_t nr;

	nr = read(cfd, req, REQ_BUF_SIZE);
	if (nr <= 0)
		return;
	req[nr] = '\0';
	(void)sscanf(req, "%15s %255s", method, path);

	if (!strcmp(method, "GET") &&
	    (!strcmp(path, "/") || !strcmp(path, "/admin") ||
	     !strcmp(path, "/admin_server_control.html"))) {
		send_response(cfd, "200 OK", "text/html; charset=utf-8",
			      html, html_len);
		return;
	}
	if (!strcmp(method, "GET") && !strcmp(path, "/api/admin/state")) {
		send_state_json(cfd, cfg);
		return;
	}
	if (!strcmp(method, "POST") && !strcmp(path, "/api/admin/scale")) {
		const char *resp = "{\"ok\":true}";
		syslog(LOG_NOTICE, "admin_ui scale request accepted");
		send_response(cfd, "200 OK", "application/json", resp,
			      strlen(resp));
		return;
	}

	send_response(cfd, "404 Not Found", "application/json",
		      "{\"error\":\"not found\"}", strlen("{\"error\":\"not found\"}"));
}

int main(int argc, char **argv)
{
	const char *config_path = "/etc/cluster_os/config.conf";
	const char *html_path = "tools/cluster_os/admin_server_control.html";
	struct cluster_os_config cfg;
	char errbuf[256];
	char *html = NULL;
	size_t html_len = 0;
	int listen_port = 0;
	int opt;
	int sfd = -1;
	struct sockaddr_in addr;
	int one = 1;

	while ((opt = getopt(argc, argv, "c:p:w:h")) != -1) {
		switch (opt) {
		case 'c':
			config_path = optarg;
			break;
		case 'p':
			listen_port = atoi(optarg);
			break;
		case 'w':
			html_path = optarg;
			break;
		case 'h':
		default:
			usage(argv[0]);
			return opt == 'h' ? 0 : 2;
		}
	}

	if (cluster_os_load_config(config_path, &cfg, errbuf, sizeof(errbuf))) {
		fprintf(stderr, "cluster_os_admin: invalid config '%s': %s\n",
			config_path, errbuf);
		return 1;
	}
	if (cfg.general.role != CLUSTER_OS_ROLE_ADMIN) {
		fprintf(stderr, "cluster_os_admin: role must be 'admin'\n");
		return 1;
	}
	if (!listen_port)
		listen_port = cfg.websocket.port > 0 ? cfg.websocket.port : 9000;

	if (load_html(html_path, &html, &html_len)) {
		fprintf(stderr, "cluster_os_admin: unable to load ui '%s'\n",
			html_path);
		return 1;
	}

	openlog("cluster_os_admin", LOG_PID | LOG_CONS, LOG_DAEMON);
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		perror("socket");
		free(html);
		return 1;
	}
	(void)setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short)listen_port);
	if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(sfd);
		free(html);
		return 1;
	}
	if (listen(sfd, 16) < 0) {
		perror("listen");
		close(sfd);
		free(html);
		return 1;
	}

	syslog(LOG_INFO, "admin ui app started on port %d", listen_port);
	while (!stop_server) {
		int cfd = accept(sfd, NULL, NULL);
		if (cfd < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		handle_client(cfd, &cfg, html, html_len);
		close(cfd);
	}

	syslog(LOG_INFO, "admin ui app stopping");
	close(sfd);
	closelog();
	free(html);
	return 0;
}
