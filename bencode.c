#include "bencode.h"

int decodedict(char *buf, int len, char *key, char **pval, int *vallen) {
  char *p, *endptr;
  int numflag = 0;
  int valflag = 0;
  int num;
  int dictlevel = 0;
  int keycmp = 0;
  int intflag = 0;

  p = buf;
  while (p < buf + len) {
    if (*p == 'd' || *p == 'l') {
      valflag = 0;
      dictlevel++;
      p++;
      continue;
    }
    if (*p == 'e') {
      if (intflag == 1) {
        intflag = 0;
      } else {
        dictlevel--;
      }

      valflag = 0;
      p++;
      continue;
    }

    if (intflag == 1) {
      p++;
      continue;
    }
    if (*p == 'i') {
      intflag = 1;
      p++;
      continue;
    }

    if (*p >= '0' && *p <= '9') {
      num = strtol(p, &endptr, 10);
      if (num == LONG_MIN || num == LONG_MAX) {
        perror("strtol, decodedict\n");
        return -1;
      }
      numflag = 1;
      p = endptr;
      continue;
    }

    if (*p == ':') {
      if (numflag == 1) {
        p++;
        numflag = 0;
        if (valflag == 1) {
          if (keycmp == 0) {
            *pval = p;
            *vallen = num;
            return 0;
          } else {
            p = p + num;
            valflag = 1 - valflag;
            continue;
          }
        } else {
          keycmp = memcmp(p, key, num);
          p = p + num;
          valflag = 1 - valflag;
          continue;
        }
      } else {
        fprintf(stderr, "decodedict, not numflag\n");
        return -1;
      }
    }

    fprintf(stderr, "*p not expected, c: %c, pos: %ld\n", *p, p - buf);
    return -1;
  }

  if (dictlevel == 0) {
    pval = NULL;
    *vallen = 0;
    return 0;
  } else {
    fprintf(stderr, "decodedict, level not 0\n");
    return -1;
  }
}

char *inttostring(int *plen) {
  int size;
  char *str;

  size = snprintf(NULL, 0, "%d", *plen);
  if (size < 0) {
    perror("snprintf, concatstring, 1");
    exit(EXIT_FAILURE);
  }
  size++;
  str = malloc(size);
  if (str == NULL) {
    perror("malloc, inttostring");
    exit(EXIT_FAILURE);
  }

  size = snprintf(str, size, "%d", *plen);
  if (size < 0) {
    perror("snprintf, concatstring, 2");
    free(str);
    exit(EXIT_FAILURE);
  }

  *plen = size;
  return str;
}

char *encodestring(char *res, int *preslen, char *str, int len) {
  int strlen = len;
  char *numstr;

  numstr = inttostring(&len);
  if (numstr == NULL) {
    fprintf(stderr, "inttostring, encodestring\n");
    fflush(stdout);
    exit(EXIT_FAILURE);
  }

  res = realloc(res, *preslen + len + 1 + strlen);
  if (res == NULL) {
    perror("realloc, encodestring\n");
    exit(EXIT_FAILURE);
  }

  memcpy(res + *preslen, numstr, len);
  free(numstr);
  res[*preslen + len] = ':';
  memcpy(res + *preslen + len + 1, str, strlen);

  *preslen = *preslen + len + 1 + strlen;

  return res;
}

char *encodedictstart(char *res, int *preslen) {
  res = realloc(res, *preslen + 1);
  if (res == NULL) {
    perror("realloc, encodedictstart\n");
    exit(EXIT_FAILURE);
  }
  res[*preslen] = 'd';
  *preslen = *preslen + 1;
  return res;
}

char *encodedictend(char *res, int *preslen) {
  res = realloc(res, *preslen + 1);
  if (res == NULL) {
    perror("realloc, encodedictend\n");
    exit(EXIT_FAILURE);
  }
  res[*preslen] = 'e';
  *preslen = *preslen + 1;
  return res;
}
