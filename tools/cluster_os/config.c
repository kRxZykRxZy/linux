// SPDX-License-Identifier: GPL-2.0
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

static bool parse_bool(const char *value, bool *out)
{
if (!strcmp(value, "true") || !strcmp(value, "1")) {
*out = true;
return true;
}
if (!strcmp(value, "false") || !strcmp(value, "0")) {
*out = false;
return true;
}
return false;
}

static void trim_inplace(char *s)
{
	char *start = s;
	char *end;

	while (isspace((unsigned char)*start))
		start++;

	if (start != s)
		memmove(s, start, strlen(start) + 1);
	if (*s == '\0')
		return;

	end = s + strlen(s) - 1;
	while (end > s && isspace((unsigned char)*end))
		end--;
	end[1] = '\0';
}

static void strip_quotes(char *s)
{
size_t len = strlen(s);

if (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') ||
 (s[0] == '\'' && s[len - 1] == '\''))) {
memmove(s, s + 1, len - 2);
s[len - 2] = '\0';
}
}

const char *cluster_os_role_to_string(enum cluster_os_role role)
{
switch (role) {
case CLUSTER_OS_ROLE_ORCHESTRING:
return "orchestring";
case CLUSTER_OS_ROLE_JOB_CLUSTER_SERVER:
return "job_cluster_server";
case CLUSTER_OS_ROLE_ADMIN:
return "admin";
default:
return "invalid";
}
}

enum cluster_os_role cluster_os_role_from_string(const char *value)
{
if (!value)
return CLUSTER_OS_ROLE_INVALID;
if (!strcmp(value, "orchestring"))
return CLUSTER_OS_ROLE_ORCHESTRING;
if (!strcmp(value, "job_cluster_server"))
return CLUSTER_OS_ROLE_JOB_CLUSTER_SERVER;
if (!strcmp(value, "admin"))
return CLUSTER_OS_ROLE_ADMIN;
return CLUSTER_OS_ROLE_INVALID;
}

void cluster_os_config_init_defaults(struct cluster_os_config *cfg)
{
memset(cfg, 0, sizeof(*cfg));

cfg->general.role = CLUSTER_OS_ROLE_INVALID;
strncpy(cfg->general.locale, "en_US", sizeof(cfg->general.locale) - 1);
strncpy(cfg->general.timezone, "UTC", sizeof(cfg->general.timezone) - 1);

cfg->network.enable_dynamic_ip = true;
cfg->network.auto_assign_ip = true;
strncpy(cfg->network.base_subnet, "10.10.0.0/16",
sizeof(cfg->network.base_subnet) - 1);
strncpy(cfg->network.interface_name, "eth0",
sizeof(cfg->network.interface_name) - 1);
cfg->network.discovery_port = 9200;
strncpy(cfg->network.discovery_protocol, "tcp",
sizeof(cfg->network.discovery_protocol) - 1);
cfg->network.max_retries = 5;

cfg->websocket.port = 9000;
cfg->websocket.tls_enabled = true;
strncpy(cfg->websocket.tls_cert, "/etc/cluster_os/certs/cert.pem",
sizeof(cfg->websocket.tls_cert) - 1);
strncpy(cfg->websocket.tls_key, "/etc/cluster_os/certs/key.pem",
sizeof(cfg->websocket.tls_key) - 1);
cfg->websocket.max_connections = 200;
cfg->websocket.heartbeat_interval_sec = 10;
cfg->websocket.reconnect_interval_sec = 5;
cfg->websocket.compression = true;
cfg->websocket.gui_notifications = true;

cfg->role_limits.telemetry_interval_sec = 5;
}

static int parse_line(char *line, struct cluster_os_config *cfg,
      char *errbuf, int errbuf_len)
{
char *sep;
char *key;
char *val;
long num;
char *endptr;
bool bval;

if (!line[0] || line[0] == '#')
return 0;

sep = strchr(line, ':');
if (!sep)
sep = strchr(line, '=');
if (!sep)
return 0;

*sep = '\0';
key = line;
val = sep + 1;
trim_inplace(key);
trim_inplace(val);
strip_quotes(val);

if (!strcmp(key, "general.node_id")) {
strncpy(cfg->general.node_id, val, sizeof(cfg->general.node_id) - 1);
return 0;
}
if (!strcmp(key, "general.hostname")) {
strncpy(cfg->general.hostname, val,
sizeof(cfg->general.hostname) - 1);
return 0;
}
if (!strcmp(key, "general.role")) {
cfg->general.role = cluster_os_role_from_string(val);
return 0;
}
if (!strcmp(key, "general.locale")) {
strncpy(cfg->general.locale, val, sizeof(cfg->general.locale) - 1);
return 0;
}
if (!strcmp(key, "general.timezone")) {
strncpy(cfg->general.timezone, val, sizeof(cfg->general.timezone) - 1);
return 0;
}
if (!strcmp(key, "network.base_subnet")) {
strncpy(cfg->network.base_subnet, val,
sizeof(cfg->network.base_subnet) - 1);
return 0;
}
if (!strcmp(key, "network.interface")) {
strncpy(cfg->network.interface_name, val,
sizeof(cfg->network.interface_name) - 1);
return 0;
}
if (!strcmp(key, "network.discovery_protocol")) {
strncpy(cfg->network.discovery_protocol, val,
sizeof(cfg->network.discovery_protocol) - 1);
return 0;
}
if (!strcmp(key, "network.enable_dynamic_ip")) {
if (!parse_bool(val, &bval)) {
snprintf(errbuf, errbuf_len,
 "invalid boolean for network.enable_dynamic_ip");
return -EINVAL;
}
cfg->network.enable_dynamic_ip = bval;
return 0;
}
if (!strcmp(key, "network.auto_assign_ip")) {
if (!parse_bool(val, &bval)) {
snprintf(errbuf, errbuf_len,
 "invalid boolean for network.auto_assign_ip");
return -EINVAL;
}
cfg->network.auto_assign_ip = bval;
return 0;
}
if (!strcmp(key, "network.discovery_port") ||
    !strcmp(key, "network.max_retries") ||
    !strcmp(key, "websocket.port") ||
    !strcmp(key, "websocket.max_connections") ||
    !strcmp(key, "websocket.heartbeat_interval_sec") ||
    !strcmp(key, "websocket.reconnect_interval_sec") ||
    !strcmp(key, "orchestring.max_cpu") ||
    !strcmp(key, "orchestring.max_gpu") ||
    !strcmp(key, "orchestring.telemetry_interval_sec") ||
    !strcmp(key, "job_cluster_server.max_cpu") ||
    !strcmp(key, "job_cluster_server.max_gpu") ||
    !strcmp(key, "job_cluster_server.telemetry_interval_sec") ||
    !strcmp(key, "admin.telemetry_interval_sec")) {
errno = 0;
num = strtol(val, &endptr, 10);
if (errno || *endptr) {
snprintf(errbuf, errbuf_len,
 "invalid integer for key '%s'", key);
return -EINVAL;
}
if (!strcmp(key, "network.discovery_port"))
cfg->network.discovery_port = (int)num;
else if (!strcmp(key, "network.max_retries"))
cfg->network.max_retries = (int)num;
else if (!strcmp(key, "websocket.port"))
cfg->websocket.port = (int)num;
else if (!strcmp(key, "websocket.max_connections"))
cfg->websocket.max_connections = (int)num;
else if (!strcmp(key, "websocket.heartbeat_interval_sec"))
cfg->websocket.heartbeat_interval_sec = (int)num;
else if (!strcmp(key, "websocket.reconnect_interval_sec"))
cfg->websocket.reconnect_interval_sec = (int)num;
else if (!strcmp(key, "orchestring.max_cpu") ||
 !strcmp(key, "job_cluster_server.max_cpu"))
cfg->role_limits.max_cpu = (int)num;
else if (!strcmp(key, "orchestring.max_gpu") ||
 !strcmp(key, "job_cluster_server.max_gpu"))
cfg->role_limits.max_gpu = (int)num;
else
cfg->role_limits.telemetry_interval_sec = (int)num;
return 0;
}
	if (!strcmp(key, "websocket.tls_enabled") ||
	    !strcmp(key, "websocket.compression") ||
	    !strcmp(key, "admin.gui.notifications")) {
		if (!parse_bool(val, &bval)) {
			snprintf(errbuf, errbuf_len,
				 "invalid boolean for key '%s'", key);
			return -EINVAL;
		}
	if (!strcmp(key, "websocket.tls_enabled"))
		cfg->websocket.tls_enabled = bval;
	else if (!strcmp(key, "websocket.compression"))
		cfg->websocket.compression = bval;
	else
		cfg->websocket.gui_notifications = bval;
	return 0;
}
if (!strcmp(key, "websocket.tls_cert")) {
strncpy(cfg->websocket.tls_cert, val,
sizeof(cfg->websocket.tls_cert) - 1);
return 0;
}
if (!strcmp(key, "websocket.tls_key")) {
strncpy(cfg->websocket.tls_key, val,
sizeof(cfg->websocket.tls_key) - 1);
return 0;
}
if (!strcmp(key, "orchestring.max_memory") ||
    !strcmp(key, "job_cluster_server.max_memory")) {
strncpy(cfg->role_limits.max_memory, val,
sizeof(cfg->role_limits.max_memory) - 1);
return 0;
}

if (!strcmp(key, "admin.auth_tokens")) {
char *token;
char tmp[CLUSTER_OS_MAX_STR * 2];
char *saveptr = NULL;

strncpy(tmp, val, sizeof(tmp) - 1);
tmp[sizeof(tmp) - 1] = '\0';
token = strtok_r(tmp, ",", &saveptr);
cfg->websocket.auth_token_count = 0;
while (token && cfg->websocket.auth_token_count < CLUSTER_OS_MAX_TOKENS) {
trim_inplace(token);
strip_quotes(token);
strncpy(cfg->websocket.auth_tokens[cfg->websocket.auth_token_count],
token, CLUSTER_OS_MAX_STR - 1);
cfg->websocket.auth_token_count++;
token = strtok_r(NULL, ",", &saveptr);
}
return 0;
}

return 0;
}

int cluster_os_load_config(const char *path, struct cluster_os_config *cfg,
   char *errbuf, int errbuf_len)
{
FILE *f;
char line[1024];
int rc = 0;
int lineno = 0;

cluster_os_config_init_defaults(cfg);

f = fopen(path, "re");
if (!f) {
snprintf(errbuf, errbuf_len, "unable to open config '%s': %s",
 path, strerror(errno));
return -errno;
}

while (fgets(line, sizeof(line), f)) {
char *nl = strchr(line, '\n');
lineno++;
if (nl)
*nl = '\0';
rc = parse_line(line, cfg, errbuf, errbuf_len);
if (rc) {
size_t used = strlen(errbuf);
snprintf(errbuf + used, errbuf_len - used,
 " (line %d)", lineno);
break;
}
}

if (fclose(f) && !rc)
rc = -EIO;

if (rc)
return rc;

return cluster_os_config_validate(cfg, errbuf, errbuf_len);
}

int cluster_os_config_validate(const struct cluster_os_config *cfg,
      char *errbuf, int errbuf_len)
{
if (!cfg->general.node_id[0]) {
snprintf(errbuf, errbuf_len, "general.node_id is required");
return -EINVAL;
}
if (!cfg->general.hostname[0]) {
snprintf(errbuf, errbuf_len, "general.hostname is required");
return -EINVAL;
}
if (cfg->general.role == CLUSTER_OS_ROLE_INVALID) {
snprintf(errbuf, errbuf_len,
 "general.role must be one of orchestring|job_cluster_server|admin");
return -EINVAL;
}
if (cfg->network.discovery_port <= 0 || cfg->network.discovery_port > 65535) {
snprintf(errbuf, errbuf_len,
 "network.discovery_port must be between 1 and 65535");
return -EINVAL;
}
if (strcmp(cfg->network.discovery_protocol, "tcp") &&
    strcmp(cfg->network.discovery_protocol, "udp") &&
    strcmp(cfg->network.discovery_protocol, "websocket")) {
snprintf(errbuf, errbuf_len,
 "network.discovery_protocol must be tcp|udp|websocket");
return -EINVAL;
}
if (cfg->websocket.tls_enabled &&
    (!cfg->websocket.tls_cert[0] || !cfg->websocket.tls_key[0])) {
snprintf(errbuf, errbuf_len,
 "websocket.tls_cert and websocket.tls_key are required when TLS is enabled");
return -EINVAL;
}
if (cfg->general.role == CLUSTER_OS_ROLE_ADMIN &&
    cfg->websocket.auth_token_count <= 0) {
snprintf(errbuf, errbuf_len,
 "admin.auth_tokens must include at least one token");
return -EINVAL;
}

return 0;
}
