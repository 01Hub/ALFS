#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libcomm.h>
#include <secret.h>
#include <ssl.h>

typedef struct
{
	char *host, *dir;
	int port;
} ssh_info;

sock srv, cl;

static void *spawn_srv (void *inf)
{
	char cmd[512];
	FILE *conn;
	ssh_info *info = (ssh_info *)inf;

	secret = gen_secret();
	
	snprintf(cmd, 512, "ssh %s \"cd %s;./alfsd %s %i %s 2>&1\"", info->host, 
		info->dir, info->host, info->port, secret);
	/*snprintf(cmd, 512, "ssh %s \"cd %s;echo -e 'run %s %i %s\nbt\nquit' >gdb.exe\"",
		info->host, info->dir, info->host, info->port, secret);
	system(cmd);
	snprintf(cmd, 512, "ssh %s \"cd %s;gdb -x gdb.exe ./alfsd\"", 
		info->host, info->dir);*/
	
	conn = popen(cmd, "r");
	if (!conn)
		return NULL;
	while (fgets(cmd, 512, conn))
		fprintf(stderr, "%s", cmd);
	pclose(conn);
	return NULL;
}

int comm_init (char *host, char *dir)
{
	pthread_t thr;
	ssh_info info = { host: host, dir: dir, port: 0 };
	
	sock_open(&srv, "127.0.0.1", 0);
	if (sock_bind(&srv))
		return -1;

	sock_listen(&srv, 1);
	info.port=ntohs(srv.in_adr.sin_port);
	pthread_create(&thr, NULL, spawn_srv, &info);
	sock_accept(&srv, &cl);
	ssl_init();
	if (!check_secret())
		return -1;
	return 0;
}

int comm_deinit ()
{
	while (comm_rdch()!=MANGO);
	sock_close(&cl, false);
	sock_close(&srv, false);
	ssl_deinit();
	return 0;
}

int comm_srv_init (int argc, char **argv)
{
	if (argc<4)
		return -1;
	
	setvbuf(stdout, NULL, _IONBF, 0);
	sock_open(&cl, argv[1], atoi(argv[2]));
	if (sock_con(&cl))
		return -1;
	ssl_srv_init();
	if (strlen(argv[3])<(SEC_LEN-1)*2)
		return -1;
	SSL_write(ssl, argv[3], SEC_LEN);
	return 0;
}

int comm_srv_deinit ()
{
	usleep(10);
	comm_wrch(MANGO);
	sock_close(&cl, false);
	return ssl_srv_deinit();
}
