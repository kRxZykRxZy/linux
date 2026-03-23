// SPDX-License-Identifier: GPL-2.0
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../config.h"

static void write_file(const char *path, const char *content)
{
FILE *f = fopen(path, "we");

assert(f);
assert(fputs(content, f) >= 0);
assert(fclose(f) == 0);
}

static void test_valid_admin_with_notifications(void)
{
struct cluster_os_config cfg;
char err[256];
char path[] = "/tmp/cluster_os_test_adminXXXXXX";
int fd = mkstemp(path);

assert(fd >= 0);
close(fd);

write_file(path,
   "general.node_id=node-a\n"
   "general.hostname=node-a.local\n"
   "general.role=admin\n"
   "network.discovery_protocol=websocket\n"
   "admin.auth_tokens=secret-token-1,secret-token-2\n"
   "admin.gui.notifications=true\n");

assert(cluster_os_load_config(path, &cfg, err, sizeof(err)) == 0);
assert(cfg.general.role == CLUSTER_OS_ROLE_ADMIN);
assert(cfg.websocket.auth_token_count == 2);
assert(cfg.websocket.gui_notifications == true);

unlink(path);
}

static void test_invalid_role_rejected(void)
{
struct cluster_os_config cfg;
char err[256];
char path[] = "/tmp/cluster_os_test_roleXXXXXX";
int fd = mkstemp(path);

assert(fd >= 0);
close(fd);

write_file(path,
   "general.node_id=node-b\n"
   "general.hostname=node-b.local\n"
   "general.role=wrong\n");

assert(cluster_os_load_config(path, &cfg, err, sizeof(err)) == -EINVAL);

unlink(path);
}

static void test_leading_space_key_parsing(void)
{
struct cluster_os_config cfg;
char err[256];
char path[] = "/tmp/cluster_os_test_spaceXXXXXX";
int fd = mkstemp(path);

assert(fd >= 0);
close(fd);

write_file(path,
   "  general.node_id = node-c\n"
   "\tgeneral.hostname = node-c.local\n"
   " general.role = job_cluster_server\n"
   " network.discovery_protocol = tcp\n");

assert(cluster_os_load_config(path, &cfg, err, sizeof(err)) == 0);
assert(!strcmp(cfg.general.node_id, "node-c"));
assert(cfg.general.role == CLUSTER_OS_ROLE_JOB_CLUSTER_SERVER);

unlink(path);
}

int main(void)
{
test_valid_admin_with_notifications();
test_invalid_role_rejected();
test_leading_space_key_parsing();
printf("cluster_os config tests: OK\n");
return 0;
}
