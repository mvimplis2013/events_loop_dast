#include <sys/un.h>
#include <sys/socket.h>
#include <ev.h>
#include <stdlib.h>
#include <stdio.h>

#include "array-heap.h"

struct sock_ev_serv {
  ev_io io;
  int fd;
  struct sockaddr_un socket;
  int socket_len;
  array clients;
};

struct socket_ev_client {
  ev_io io;
  int fd;
  int index;
  struct sock_ev_serv *server;
};

static void client_cb(EV_P_ ev_io *w, int revents) {
  // A client has become readable
  struct socket_ev_client* client = (struct socket_ev_client*)w;

  int n;

  printf("[r]");
  n = recv(client->fd, &data, sizeof data, 0);

  if (n <= 0) {
    if (n == 0) {
      puts("orderly disconnect");
      ev_io_stop(EV_A_ &client->io);
      close(client->fd);
    } else if (EAGAIN == errno) {
      puts("should never get in this state");
    } else {
      perror("recv");
    }

    return;
  }

  if (send(client->fd, ".", strlen("."), 0) < 0) {
    perror("send");
  }
}

inline static struct socket_ev_client* client_new(int fd) {
    struct socket_ev_client *client;
    client = realloc(NULL, sizeof(struct socket_ev_client));
    setnonblock(client->fd);
    ev_io_init(&client->io, client_cb, client->fd, EV_READ);

    return client;
}

static void server_cb(EV_P_ ev_io *w, int revents) {
  puts("unix stream socket has become READable");

  int client_fd;
  struct socket_ev_client* client;

  struct socket_ev_serv* server = (struct socket_ev_serv*)w;

  while (1) {
    client_fd = accept(server->fd, NULL, NULL);
    if (client_fd == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        g_warning("accept() failed errno=%i (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;
    }
    puts("accepted a client");
    client = client_new(client_fd);
    client->server = server;
    client->index = array_push(&server->clients, client);
    ev_io_start(EV_A_ &client->io);
  }
}

int unix_socket_init(struct sockaddr_un *socket_un, char *sock_path, int max_queue) {
  int fd;

  unlink(sock_path);

  // Setup a unix socket listener
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("server socket");
    exit(EXIT_FAILURE);
  }

  // Set-it in non-blocking mode
  if (setnonblock(fd) == -1) {
    perror("server socket nonblock");
    exit(EXIT_FAILURE);
  }

  // Set it as a Unix socket
  socket_un->sun_family = AF_UNIX;
  strcpy(socket_un->sun_path, sock_path);

  return fd;
}

int server_init(struct sock_ev_serv* server, char* sock_path, int max_queue) {
  server->fd = unix_socket_init(&server->socket, sock_path, max_queue);
  server->socket_len = sizeof(server->socket.sun_family) + strlen(server->socket.sun_path);

  array_init(&server->clients, 128);

  if (bind(server->fd, (struct sockaddr*) &server->socket, server->socket_len)) {
    perror("server bind");
    exit(EXIT_FAILURE);
  }

  if (listen(server->fd, max_queue) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return 0;
}

int main(void) {
  int max_queue = 128;

  struct sock_ev_serv server;
  struct ev_periodic every_few_seconds;

  // Create our single-loop for this single-thread application
  EV_P = ev_default_loop(0);

  // Create unix socket in non-blocking mode
  server_init(&server, "/tmp/libev-ipc-daemon.sock", max_queue);

  ev_periodic_init(&every_few_seconds, not_blocked, 0, 5, 0);
  ev_period_start(EV_A_ &every_few_seconds);

  // Get notified ... socket is ready to ready
  ev_io_init(&server.io, server_cb, server.fd, EV_READ);
  ev_io_start(EV_A_ &server.io);

  // Run our loop ... forever
  puts("unix-socket-echo starting ...\n");
  ev_loop(EV_A_ 0);

  // This point is only ever reached if ... loop is manually exited
  close(server.fd);

  return EXIT_SUCCESS;
}
