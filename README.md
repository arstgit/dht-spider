# dhtspider

[![Build Status](https://travis-ci.org/derekchuank/dhtspider.svg?branch=master)](https://travis-ci.org/derekchuank/dhtspider)

dhtspider.

## Compile & Install

```
  $ make
  $ make install
```

## Usage

In order to read result from stdout, simply run:
```
  $ dhtspider
```

Or, make result piped to a `fifo` file:
```
  $ dhtspider fifo
```
The fifo file is located in /var/dhtspider/, named `fifo`.

## Traffic control

`tc` is used with docker to control the out traffic, you can modify `Dockerfile` and `docker-compose.yml` for more sophisticated control.
