/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CLUSTER_OS_CONFIG_H
#define CLUSTER_OS_CONFIG_H

#include <stdbool.h>

#define CLUSTER_OS_MAX_STR 256
#define CLUSTER_OS_MAX_TOKENS 8

enum cluster_os_role {
CLUSTER_OS_ROLE_ORCHESTRING,
CLUSTER_OS_ROLE_JOB_CLUSTER_SERVER,
CLUSTER_OS_ROLE_ADMIN,
CLUSTER_OS_ROLE_INVALID,
};

struct cluster_os_general_config {
char node_id[CLUSTER_OS_MAX_STR];
char hostname[CLUSTER_OS_MAX_STR];
enum cluster_os_role role;
char locale[CLUSTER_OS_MAX_STR];
char timezone[CLUSTER_OS_MAX_STR];
};

struct cluster_os_network_config {
bool enable_dynamic_ip;
bool auto_assign_ip;
char base_subnet[CLUSTER_OS_MAX_STR];
char interface_name[CLUSTER_OS_MAX_STR];
int discovery_port;
char discovery_protocol[CLUSTER_OS_MAX_STR];
int max_retries;
};

struct cluster_os_websocket_config {
int port;
bool tls_enabled;
char tls_cert[CLUSTER_OS_MAX_STR];
char tls_key[CLUSTER_OS_MAX_STR];
int max_connections;
int heartbeat_interval_sec;
int reconnect_interval_sec;
bool compression;
bool gui_notifications;
char auth_tokens[CLUSTER_OS_MAX_TOKENS][CLUSTER_OS_MAX_STR];
int auth_token_count;
};

struct cluster_os_role_limits {
int max_cpu;
char max_memory[CLUSTER_OS_MAX_STR];
int max_gpu;
int telemetry_interval_sec;
};

struct cluster_os_config {
struct cluster_os_general_config general;
struct cluster_os_network_config network;
struct cluster_os_websocket_config websocket;
struct cluster_os_role_limits role_limits;
};

const char *cluster_os_role_to_string(enum cluster_os_role role);
enum cluster_os_role cluster_os_role_from_string(const char *value);

void cluster_os_config_init_defaults(struct cluster_os_config *cfg);
int cluster_os_load_config(const char *path, struct cluster_os_config *cfg,
   char *errbuf, int errbuf_len);
int cluster_os_config_validate(const struct cluster_os_config *cfg,
      char *errbuf, int errbuf_len);

#endif
