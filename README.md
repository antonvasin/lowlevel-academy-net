# Low-Level Academy: [Netcode That Doesn't Suck](https://lowlevel.academy/player/2)

Implementing simple networked DB using multiplexing.

```
make
./bin/dbserver -f ./mynewdb.db -p 5555
./bin/dbcli -h 127.0.0.1 -p 5555 -l
```

## Blocking Requests

![blocking](./blocking.png)

## `select()`

![select](./select.png)

## `poll()`

![poll](./poll.png)

