#include "core.h"

struct node {
  char one[26];
};

struct nodearr {
  int start;
  int end;
  struct node item[NODE_SIZE];
} nodes = {0, 0};

struct startnode {
  char address[255];
  char port[10];
} startnode1 = {"router.bittorrent.com", "6881"},
  startnode2 = {"dht.transmissionbt.com", "6881"};

char hostnodeid[20];
char neighborid[20];
char randomid[20];
char nodes8[208];
char hash[20];
char tid[2];
char token[8];
int nodesfullflag = 0;

char *neighbor(char *id) {
  memcpy(neighborid, id, 6);
  memcpy(neighborid + 6, hostnodeid, 14);
  return neighborid;
}

static char *gettoken() {
  int random;
  time_t now;

  now = time(NULL);
  if (now == -1) {
    perror("time, gettoken");
    exit(EXIT_FAILURE);
  }

  srand((unsigned int)now);

  for (int i = 0; i < 8; i++) {
    random = rand();
    token[i] = random % 10 + '0';
  }

  return token;
}

static char *getrandomid(char *str) {
  int random;
  time_t now;

  now = time(NULL);
  if (now == -1) {
    perror("time, getrandomid");
    exit(EXIT_FAILURE);
  }

  srand((unsigned int)now);

  for (int i = 0; i < 10; i++) {
    random = rand();
    memcpy(str + i * 2, &random, 2);
  }

  return str;
}

static char *gettid() {
  int random;
  time_t now;

  now = time(NULL);
  if (now == -1) {
    perror("time, gettid");
    exit(EXIT_FAILURE);
  }

  srand((unsigned int)now);

  for (int i = 0; i < 2; i++) {
    random = rand();
    tid[i] = random % 10 + '0';
  }

  return tid;
}

char *buildping(char *res, int *preslen, char *id, char *tid, int tidlen) {
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "r", 1);
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "id", 2);
  res = encodestring(res, preslen, id, 20);
  res = encodedictend(res, preslen);

  res = encodestring(res, preslen, "t", 1);
  res = encodestring(res, preslen, tid, tidlen);
  res = encodestring(res, preslen, "y", 1);
  res = encodestring(res, preslen, "r", 1);
  res = encodedictend(res, preslen);
}

char *buildannouncepeer(char *res, int *preslen, char *id, char *tid,
                        int tidlen) {
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "r", 1);
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "id", 2);
  res = encodestring(res, preslen, id, 20);
  res = encodedictend(res, preslen);

  res = encodestring(res, preslen, "t", 1);
  res = encodestring(res, preslen, tid, tidlen);
  res = encodestring(res, preslen, "y", 1);
  res = encodestring(res, preslen, "r", 1);
  res = encodedictend(res, preslen);
}

char *buildgetpeers(char *res, int *preslen, char *id, char *tid, int tidlen,
                    char *curnode) {
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "r", 1);
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "id", 2);
  res = encodestring(res, preslen, id, 20);
  res = encodestring(res, preslen, "nodes", 5);
  res = encodestring(res, preslen, curnode, 208);
  res = encodestring(res, preslen, "token", 5);
  gettoken();
  res = encodestring(res, preslen, token, 5);
  res = encodedictend(res, preslen);

  res = encodestring(res, preslen, "t", 1);
  res = encodestring(res, preslen, tid, tidlen);
  res = encodestring(res, preslen, "y", 1);
  res = encodestring(res, preslen, "r", 1);
  res = encodedictend(res, preslen);
}

char *buildrespondfindnode(char *res, int *preslen, char *id, char *tid,
                           int tidlen, char *curnode) {
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "r", 1);
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "id", 2);
  res = encodestring(res, preslen, id, 20);
  res = encodestring(res, preslen, "nodes", 5);
  res = encodestring(res, preslen, curnode, 208);
  res = encodedictend(res, preslen);

  res = encodestring(res, preslen, "t", 1);
  res = encodestring(res, preslen, tid, tidlen);
  res = encodestring(res, preslen, "y", 1);
  res = encodestring(res, preslen, "r", 1);
  res = encodedictend(res, preslen);
}

char *buildfindnode(char *res, int *preslen, char *id) {
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "a", 1);
  res = encodedictstart(res, preslen);
  res = encodestring(res, preslen, "id", 2);
  res = encodestring(res, preslen, id, 20);
  res = encodestring(res, preslen, "target", 6);
  getrandomid(randomid);
  res = encodestring(res, preslen, randomid, 20);
  res = encodedictend(res, preslen);

  res = encodestring(res, preslen, "q", 1);
  res = encodestring(res, preslen, "find_node", 9);
  res = encodestring(res, preslen, "t", 1);
  gettid();
  res = encodestring(res, preslen, tid, 2);
  res = encodestring(res, preslen, "y", 1);
  res = encodestring(res, preslen, "q", 1);
  res = encodedictend(res, preslen);

  return res;
}

int sendbencode(char *res, int len, struct sockaddr *addr, socklen_t addrlen) {
  ssize_t numsend;

  numsend = sendto(listenfd, res, len, 0, addr, addrlen);
  if (numsend != len) {
    eprintf("sendto, sendbencode\n");
    return -1;
  }

  return 0;
}

int respondping(char *buf, int numRead, struct sockaddr *addr,
                socklen_t addrlen) {
  ssize_t numsend;
  char *val;
  int vallen;
  char id[20];
  char tid[20];
  int tidlen;
  int s;
  char *res;
  int len;

  s = decodedict(buf, numRead, "t", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondping, t\n");
    return -1;
  }
  memcpy(tid, val, vallen);
  tidlen = vallen;

  s = decodedict(buf, numRead, "id", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondping, id\n");
    return -1;
  }
  if (vallen != 20) {
    eprintf("decodedict, respondping, id vallen\n");
    return -1;
  }
  memcpy(id, val, 20);

  res = malloc(1);
  len = 0;
  res = buildping(res, &len, neighbor(id), tid, tidlen);
  if (res == NULL) {
    eprintf("buildping\n");
    return -1;
  }

  numsend = sendto(listenfd, res, len, 0, addr, addrlen);
  if (numsend != len) {
    eprintf("sendto, sendbencode\n");
    return -1;
  }
  free(res);

  return 0;
}

int respondannouncepeer(char *buf, int numRead, struct sockaddr *addr,
                        socklen_t addrlen) {
  ssize_t numsend;
  char *val;
  int vallen;
  char id[20];
  char tid[20];
  int tidlen;
  int s;
  char *res;
  int len;

  s = decodedict(buf, numRead, "info_hash", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondannouncepeer, info_hash\n");
    return -1;
  }
  if (vallen != 20) {
    eprintf("decodedict, respondannouncepeer, infohash vallen\n");
    return -1;
  }
  memcpy(hash, val, 20);
  fprintf(stdout, "%s", MAGNET_PREFIX);
  for (int i = 0; i < 20; i++) {
    fprintf(stdout, "%02hhx", 0xff & hash[i]);
  }
  fflush(stdout);

  s = decodedict(buf, numRead, "t", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondgetpeers, t\n");
    return -1;
  }
  memcpy(tid, val, vallen);
  tidlen = vallen;

  s = decodedict(buf, numRead, "id", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondannouncepeer, id\n");
    return -1;
  }
  if (vallen != 20) {
    eprintf("decodedict, respondannouncepeer, id vallen\n");
    return -1;
  }
  memcpy(id, val, 20);

  res = malloc(1);
  len = 0;
  res = buildannouncepeer(res, &len, neighbor(id), tid, tidlen);
  if (res == NULL) {
    eprintf("buildannouncepeer\n");
    return -1;
  }

  numsend = sendto(listenfd, res, len, 0, addr, addrlen);
  if (numsend != len) {
    eprintf("sendto, sendbencode\n");
    return -1;
  }
  free(res);

  return 0;
}

int respondgetpeers(char *buf, int numRead, struct sockaddr *addr,
                    socklen_t addrlen) {
  ssize_t numsend;
  char *val;
  int vallen;
  char id[20];
  char tid[20];
  int tidlen;
  int freespace;
  int nodecnt;
  int s;
  char *res;
  int len;

  if (nodes.end - nodes.start >= 0) {
    freespace = NODE_SIZE - (nodes.end - nodes.start);
  } else {
    freespace = nodes.start - nodes.end;
  }
  nodecnt = NODE_SIZE - freespace;

  if (nodecnt < 16) {
    for (int i = 0; i < 8; i++) {
      memcpy(nodes8 + i * 26, nodes.item[0].one, 26);
    }
  } else {
    for (int i = 0; i < 8; i++) {
      memcpy(nodes8 + i * 26, nodes.item[nodes.start].one, 26);
      nodes.start++;
      if (nodes.start == NODE_SIZE) {
        nodes.start = 0;
      }
    }
  }

  s = decodedict(buf, numRead, "t", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondgetpeers, t\n");
    return -1;
  }
  memcpy(tid, val, vallen);
  tidlen = vallen;

  s = decodedict(buf, numRead, "id", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondgetpeers, id\n");
    return -1;
  }
  if (vallen != 20) {
    eprintf("decodedict, respondgetpeers, id vallen\n");
    return -1;
  }
  memcpy(id, val, 20);

  res = malloc(1);
  len = 0;
  res = buildgetpeers(res, &len, neighbor(id), tid, tidlen, nodes8);
  if (res == NULL) {
    eprintf("buildfindnode\n");
    return -1;
  }

  numsend = sendto(listenfd, res, len, 0, addr, addrlen);
  if (numsend != len) {
    eprintf("sendto, sendbencode\n");
    return -1;
  }
  free(res);

  return 0;
}

int respondfindnode(char *buf, int numRead, struct sockaddr *addr,
                    socklen_t addrlen) {
  ssize_t numsend;
  char *val;
  int vallen;
  char id[20];
  char tid[20];
  int tidlen;
  int freespace;
  int nodecnt;
  int s;
  char *res;
  int len;

  if (nodesfullflag == 0) {
    if (nodes.end - nodes.start >= 0) {
      freespace = NODE_SIZE - (nodes.end - nodes.start);
    } else {
      freespace = nodes.start - nodes.end;
    }
    nodecnt = NODE_SIZE - freespace;
  }

  if (nodesfullflag == 0 && nodecnt < 16) {
    for (int i = 0; i < 8; i++) {
      memcpy(nodes8 + i * 26, nodes.item[0].one, 26);
    }
  } else {
    for (int i = 0; i < 8; i++) {
      memcpy(nodes8 + i * 26, nodes.item[nodes.start].one, 26);
      nodes.start++;
      if (nodes.start == NODE_SIZE) {
        nodes.start = 0;
      }
    }
  }

  s = decodedict(buf, numRead, "t", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondfindnode, t\n");
    return -1;
  }
  memcpy(tid, val, vallen);
  tidlen = vallen;

  s = decodedict(buf, numRead, "id", &val, &vallen);
  fflush(stderr);
  if (s == -1 || val == NULL) {
    eprintf("decodedict: respondfindnode, id\n");
    return -1;
  }
  if (vallen != 20) {
    eprintf("decodedict, respondfindnode, id vallen\n");
    return -1;
  }
  memcpy(id, val, 20);

  res = malloc(1);
  len = 0;
  res = buildrespondfindnode(res, &len, neighbor(id), tid, tidlen, nodes8);
  if (res == NULL) {
    eprintf("buildrespondfindnode\n");
    return -1;
  }

  numsend = sendto(listenfd, res, len, 0, addr, addrlen);
  if (numsend != len) {
    eprintf("sendto, sendbencode\n");
    return -1;
  }
  free(res);

  return 0;
}

int sendfindnode(char *id, char *address, char *port) {
  char *res;
  int len;
  struct addrinfo hints;
  struct addrinfo *result;
  int s;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  hints.ai_flags = 0;
  hints.ai_protocol = 0; /* Any protocol */

  s = getaddrinfo(address, port, &hints, &result);
  if (s != 0) {
    eprintf("getaddrinfo\n");
    return -1;
  }
  if (result == NULL) {
    eprintf("getaddrinfo, result NULL\n");
    exit(EXIT_FAILURE);
  }

  res = malloc(1);
  len = 0;
  res = buildfindnode(res, &len, id);
  if (res == NULL) {
    eprintf("buildfindnode\n");
    return -1;
  }

  if (sendbencode(res, len, result->ai_addr, result->ai_addrlen) == -1) {
    eprintf("sendbencode, sendfindnode\n");
    return -1;
  }
  free(res);

  freeaddrinfo(result);
  return 0;
}

int addnode(char *p) {
  int freespace;
  int cmp;

  cmp = memcmp(p, hostnodeid, 20);
  if (cmp == 0) {
    eprintf("addnode, cmp: 0\n");
    return -1;
  }

  // if (nodes.end - nodes.start >= 0) {
  //  freespace = NODE_SIZE - (nodes.end - nodes.start);
  //} else {
  //  freespace = nodes.start - nodes.end;
  //}
  // if (freespace < 2) {
  //  eprintf("addnode, full\n");
  //  return -1;
  //}

  memcpy(nodes.item[nodes.end].one, p, 26);
  nodes.end++;
  if (nodes.end == NODE_SIZE) {
    nodesfullflag = 1;
    nodes.end = 0;
  }

  return 0;
}

int parsenodes(char *p, int len) {
  char *end = p + len;

  while (p < end) {
    if (addnode(p) == -1) {
      eprintf("addnode\n");
      return -1;
    }
    p = p + 26;
  }
}

int parsemsg(char *buf, int numRead, struct sockaddr *addr, socklen_t addrlen) {
  int res;
  int cmp;
  char *val;
  int vallen;

  res = decodedict(buf, numRead, "y", &val, &vallen);
  fflush(stderr);
  if (res == -1 || val == NULL) {
    // printf("\n");
    // for (int i = 0; i < numRead; i++)
    //  printf("%c", buf[i]);
    // printf("\n");
    // printf("numRead: %d", numRead);
    // printf("\n");
    // fflush(stdout);
    // eprintf("decodedict: y\n");
    return -1;
  }

  cmp = memcmp(val, "r", 1);
  if (cmp == 0) {
    res = decodedict(buf, numRead, "nodes", &val, &vallen);
    fflush(stderr);
    if (res == -1 || val == NULL) {
      eprintf("decodedict: nodes\n");
      return -1;
    }
    if (parsenodes(val, vallen) == -1) {
      eprintf("parsenode\n");
      return -1;
    }
    return 0;
  }

  cmp = memcmp(val, "q", 1);
  if (cmp == 0) {
    res = decodedict(buf, numRead, "q", &val, &vallen);
    fflush(stderr);
    if (res == -1 || val == NULL) {
      eprintf("decodedict: q\n");
      return -1;
    }

    cmp = memcmp(val, "get_peers", 9);
    if (cmp == 0) {
      if (respondgetpeers(buf, numRead, addr, addrlen) == -1) {
        eprintf("respondgetpeers\n");
        return -1;
      }
      return 0;
    }

    cmp = memcmp(val, "announce_peer", 13);
    if (cmp == 0) {
      if (respondannouncepeer(buf, numRead, addr, addrlen) == -1) {
        eprintf("respondannouncepeer\n");
        return -1;
      }
      return 0;
    }

    cmp = memcmp(val, "find_node", 9);
    if (cmp == 0) {
      if (respondfindnode(buf, numRead, addr, addrlen) == -1) {
        eprintf("respondfindnode\n");
        return -1;
      }
      return 0;
    }

    cmp = memcmp(val, "ping", 4);
    if (cmp == 0) {
      if (respondping(buf, numRead, addr, addrlen) == -1) {
        eprintf("respondping\n");
        return -1;
      }
      return 0;
    }
    return 0;
  }

  return 0;
}

int idle() {
  int freespace;
  int nodecnt;
  char *curnode;
  char addr[255];
  char port[10];

  if (nodesfullflag == 0) {
    if (nodes.end - nodes.start >= 0) {
      freespace = NODE_SIZE - (nodes.end - nodes.start);
    } else {
      freespace = nodes.start - nodes.end;
    }
    nodecnt = NODE_SIZE - freespace;

    if (nodecnt < 8) {
      eprintf("node not enough\n");

      if (sendfindnode(hostnodeid, startnode1.address, startnode1.port) == -1) {
        eprintf("sendfindnode, 2\n");
        return -1;
      }

      return 0;
    }
  }

  curnode = nodes.item[nodes.start].one;
  snprintf(addr, 255, "%d.%d.%d.%d", 0xff & *(curnode + 20),
           0xff & *(curnode + 21), 0xff & *(curnode + 22),
           0xff & *(curnode + 23));
  snprintf(port, 10, "%hu", ntohs(*(uint16_t *)(curnode + 24)));

  if (sendfindnode(neighbor(curnode), addr, port) == -1) {
    eprintf("sendfindnode, 1\n");
    return -1;
  }

  nodes.start++;
  if (nodes.start == NODE_SIZE) {
    nodes.start = 0;
  }

  return 0;
}

int main(int argc, char **argv) {
  getrandomid(hostnodeid);

  eloop(idle, parsemsg);
}
