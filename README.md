# Monitoring Dashboard for zdbfs

Fetch information from [0-db-fs](https://github.com/threefoldtech/0-db-fs) and display a dashboard
with realtime statistics.

Information are fetch using `ioctl()` to the filesystem and querying `NSINFO` from cache `zdb`.

# Screenshot
![Screenshot](https://i.imgur.com/QS6PPWd.png)

# Raw Data
A json is generated, from zdbfs and zdb:

```json
{
    timestamp: 1622215953,
    syscall: {
        fuse: 9828992,
        getattr: 1521503,
        setattr: 76498,
        create: 77301,
        readdir: 1365,
        open: 1563859,
        read: 1592640,
        write: 270032,
        mkdir: 5824,
        unlink: 1624,
        rmdir: 116,
        rename: 158,
        link: 0,
        symlink: 38,
        statsfs: 0,
        ioctl: 2614
    },
    cache: {
        hit: 12299401,
        miss: 83170,
        full: 0,
        linear_flush: 18,
        random_flush: 0
    },
    transfert: {
        read_bytes: 12003939136,
        write_bytes: 1098779301
    },
    metadata: {
        entries: 81418,
        index_virtual_size_bytes: 5536424,
        data_virtual_size_bytes: 5822270,
        index_physical_size_bytes: 6392418,
        data_physical_size_bytes: 14137716
    },
    blocks: {
        entries: 84772,
        index_virtual_size_bytes: 5764496,
        data_virtual_size_bytes: 2241194205,
        index_physical_size_bytes: 6626312,
        data_physical_size_bytes: 2289726108
    },
    temporary: {
        entries: 1,
        index_virtual_size_bytes: 68,
        data_virtual_size_bytes: 24,
        index_physical_size_bytes: 101,
        data_physical_size_bytes: 74
    }
}
```
