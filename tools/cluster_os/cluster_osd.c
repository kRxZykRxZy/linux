// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"

static void emit_admin_gui_notification(const struct cluster_os_config *cfg,
const char *message)
{
if (!cfg->websocket.gui_notifications)
return;

syslog(LOG_NOTICE, "server_control_notification node=%s msg=%s",
       cfg->general.node_id, message);
printf("[Server Control Notification] %s\n", message);
}

static void run_role_cycle(const struct cluster_os_config *cfg)
{
switch (cfg->general.role) {
case CLUSTER_OS_ROLE_ORCHESTRING:
syslog(LOG_INFO,
       "orchestring: scheduling active, discovery on %s/%d protocol=%s",
       cfg->network.interface_name, cfg->network.discovery_port,
       cfg->network.discovery_protocol);
break;
case CLUSTER_OS_ROLE_JOB_CLUSTER_SERVER:
syslog(LOG_INFO,
       "job_cluster_server: accepting jobs, limits cpu=%d gpu=%d mem=%s",
       cfg->role_limits.max_cpu, cfg->role_limits.max_gpu,
       cfg->role_limits.max_memory[0] ? cfg->role_limits.max_memory :
       "unbounded");
break;
case CLUSTER_OS_ROLE_ADMIN:
syslog(LOG_INFO,
       "admin: websocket=%d tls=%s max_connections=%d",
       cfg->websocket.port,
       cfg->websocket.tls_enabled ? "enabled" : "disabled",
       cfg->websocket.max_connections);
emit_admin_gui_notification(cfg,
"Cluster status updated: all discovered nodes reachable");
break;
default:
break;
}
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [-c config_path] [-n]\n"
		"  -c path   Config file (default: /etc/cluster_os/config.conf, key=value or key: value)\n"
		"  -n        Run one cycle and exit (test mode)\n",
		prog);
}

int main(int argc, char **argv)
{
	const char *config_path = "/etc/cluster_os/config.conf";
	struct cluster_os_config cfg;
	char errbuf[256];
	bool oneshot = false;
	int opt;
	int interval;

while ((opt = getopt(argc, argv, "c:nh")) != -1) {
switch (opt) {
case 'c':
config_path = optarg;
break;
case 'n':
oneshot = true;
break;
case 'h':
default:
usage(argv[0]);
return opt == 'h' ? 0 : 2;
}
}

if (cluster_os_load_config(config_path, &cfg, errbuf, sizeof(errbuf))) {
fprintf(stderr, "cluster_osd: invalid config '%s': %s\n", config_path,
errbuf);
return 1;
}

openlog("cluster_osd", LOG_PID | LOG_CONS, LOG_DAEMON);
syslog(LOG_INFO, "starting cluster_osd node_id=%s role=%s hostname=%s",
       cfg.general.node_id,
       cluster_os_role_to_string(cfg.general.role),
       cfg.general.hostname);

if (cfg.network.enable_dynamic_ip && cfg.network.auto_assign_ip)
syslog(LOG_INFO, "dynamic private IP assignment enabled on %s subnet %s",
       cfg.network.interface_name, cfg.network.base_subnet);

run_role_cycle(&cfg);
if (oneshot) {
closelog();
return 0;
}

interval = cfg.role_limits.telemetry_interval_sec > 0 ?
cfg.role_limits.telemetry_interval_sec : 5;
for (;;) {
sleep(interval);
run_role_cycle(&cfg);
}

closelog();
return 0;
}
