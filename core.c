#include "core.h"

int globalElevel = INFO_LEVEL;

void eprint(unsigned char *str, int num, ...) {
  va_list valist;
  int fd;
  int elevel;
  int perrorflag;
  time_t now;

  if (num > 3) {
    write(STDOUT_FILENO, "eprint, num\n", 12);
    exit(EXIT_FAILURE);
  }

  va_start(valist, num);

  fd = STDOUT_FILENO;
  elevel = INFO_LEVEL;
  perrorflag = 0;
  for (int i = 0; i < num; i++) {
    switch (i) {
    case 0:
      fd = va_arg(valist, int);
      break;
    case 1:
      elevel = va_arg(valist, int);
      break;
    case 2:
      perrorflag = va_arg(valist, int);
      break;
    }
  }

  if (elevel < globalElevel)
    return;

  now = time(NULL);
  if (now == -1) {
    perror("time, eprint");
    exit(EXIT_FAILURE);
  }

  if (write(fd, ctime(&now), strlen(ctime(&now)) - 1) == -1) {
    perror("write, eprint, time");
    exit(EXIT_FAILURE);
  }
  if (write(fd, " ", 1) == -1) {
    perror("write, eprint, space");
    exit(EXIT_FAILURE);
  }

  if (perrorflag == 1) {
    perror(str);
    return;
  }

  if (elevel == INFO_LEVEL) {
    if (write(fd, str, strlen(str)) == -1) {
      perror("write, eprint");
      exit(EXIT_FAILURE);
    }
  }
  if (elevel == ERR_LEVEL) {
    if (write(fd, str, strlen(str)) == -1) {
      perror("write, eprint");
      exit(EXIT_FAILURE);
    }
  }

  va_end(valist);
  return;
}

void eprintf(const char *fmt, ...) {
  int size = 0;
  char *p = NULL;
  va_list ap;

  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if (size < 0)
    return;

  size++;
  p = malloc(size);
  if (p == NULL)
    return;

  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if (size < 0) {
    free(p);
    return;
  }

  eprint(p, 1, STDERR_FILENO);

  free(p);
  return;
}

static int setnonblocking(int fd) {
  int flags;

  flags = fcntl(fd, F_GETFL);
  if (flags == -1) {
    perror("fcntl: setnonblocking, F_GETFL");
    return -1;
  }
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) < 0) {
    perror("fcntl: setnonblocking, F_SETFL");
    return -1;
  }
}

int inetPassiveSocket(const char *service, int type, socklen_t *addrlen,
                      int doListen, int backlog) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd, optval, s, flags;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  hints.ai_socktype = type;
  hints.ai_family = AF_UNSPEC; /* Allows IPv4 or IPv6 */
  hints.ai_flags = AI_PASSIVE; /* Use wildcard IP address */

  s = getaddrinfo(NULL, service, &hints, &result);
  if (s != 0)
    return -1;

  optval = 1;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1)
      continue;

    if (doListen) {
      if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) ==
          -1) {
        close(sfd);
        freeaddrinfo(result);
        return -1;
      }
    }

    if (setnonblocking(sfd) == -1) {
      close(sfd);
      sfd = -1;
      continue;
    }

    if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;

    close(sfd);
  }

  if (rp != NULL && doListen) {
    if (listen(sfd, backlog) == -1) {
      freeaddrinfo(result);
      return -1;
    }
  }

  if (rp != NULL && addrlen != NULL)
    *addrlen = rp->ai_addrlen;

  freeaddrinfo(result);

  return (rp == NULL) ? -1 : sfd;
}

int inetListen(const char *service, int backlog, socklen_t *addrlen) {
  return inetPassiveSocket(service, SOCK_DGRAM, addrlen, 0, 0);
}

void onexit(int signum) {}

int handlein(int (*parsemsg)(char *, int, struct sockaddr *, socklen_t)) {
  struct sockaddr_in6 addr;
  socklen_t addrlen;
  ssize_t numRead;

  addrlen = sizeof(struct sockaddr_in6);
  for (;;) {
    numRead = recvfrom(listenfd, buf, BUF_SIZE, 0, (struct sockaddr *)&addr,
                       &addrlen);
    if (numRead == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {
        perror("recvfrom");
        exit(EXIT_FAILURE);
      }
    }
    if (numRead == 0) {
      eprintf("recvfrom, numRead: 0");
      continue;
    }
    if (numRead > 0) {
      if (memcmp(buf, "shutdown\n", 9) == 0) {
        eprintf("shutdown\n");
        exit(EXIT_SUCCESS);
      }

      if (parsemsg(buf, numRead, (struct sockaddr *)&addr, addrlen) == -1) {
        eprintf("parsemsg\n");
      }
      continue;
    }
    perror("recvfrom, for fall\n");
    exit(EXIT_FAILURE);
  }
}

void eloop(int (*idle)(),
           int (*parsemsg)(char *, int, struct sockaddr *, socklen_t)) {
  struct epoll_event ev, evlist[MAX_EVENTS];
  struct sigaction sa;
  int efd, nfds, tfd;
  struct timespec start_time = {.tv_sec = 0, .tv_nsec = 1},
                  interval_time = {.tv_sec = FINDNODE_INTERVAL, .tv_nsec = 0};
  struct itimerspec new_time = {.it_value = start_time,
                                .it_interval = interval_time};
  uint64_t time_data;
  int numRead;

  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  if (0 && sigaction(SIGPIPE, &sa, NULL) == -1) {
    perror("failed to ignore SIGPIPE; sigaction");
    exit(EXIT_FAILURE);
  }
  sa.sa_handler = onexit;
  if (0 && sigaction(SIGINT, &sa, NULL) == -1) {
    perror("failed to ignore SIGINT; sigaction");
    exit(EXIT_FAILURE);
  }

  tfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
  if (tfd == -1) {
    perror("timerfd_create");
    exit(EXIT_FAILURE);
  }

  if (timerfd_settime(tfd, 0, &new_time, NULL) == -1) {
    perror("timerfd_settime");
    exit(EXIT_FAILURE);
  }

  listenfd = inetListen("6339", 80, NULL);
  if (listenfd == -1) {
    perror("inetListen");
    exit(EXIT_FAILURE);
  }

  efd = epoll_create1(0);
  if (efd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  ev.data.fd = listenfd;
  ev.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
    perror("epoll_ctl, EPOLL_CTL_ADD 1");
    exit(EXIT_FAILURE);
  }

  ev.data.fd = tfd;
  ev.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
    perror("epoll_ctl, EPOLL_CTL_ADD 2");
    exit(EXIT_FAILURE);
  }

  eprint("started!\n", 0);

  for (;;) {
    nfds = epoll_wait(efd, evlist, 1, EPOLL_TIMEOUT);
    // nfds = epoll_wait(efd, evlist, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      if (errno == EINTR)
        continue;
      exit(EXIT_FAILURE);
    }

    if (nfds == 0) {
      eprintf("idle\n");

      if (idle() == -1) {
        eprintf("handleidle\n");
      }
    }

    for (int n = 0; n < nfds; n++) {
      if (evlist[n].data.fd == listenfd) {
        if (handlein(parsemsg) == -1) {
          eprintf("handlein\n");
        }
      }

      if (evlist[n].data.fd == tfd) {
        eprintf("timerfd timeout\n");

        for (;;) {
          numRead = read(tfd, &time_data, sizeof(uint64_t));
          if (numRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              perror("read, timerfd");
              exit(EXIT_FAILURE);
            }
          }
          if (numRead == 0) {
            eprintf("read, numRead: 0, timerfd\n");
            break;
          }
          if (numRead > 0) {
            if (idle() == -1) {
              eprintf("handleidle\n");
            }
            continue;
          }
        }
      }
    }
  }
}
