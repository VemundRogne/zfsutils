# ZFSUTILS -- Snapper
This tool is very simple: it creates timestamped (ISO 8601) snapshots on datasets non-recursively

Usage: `snapper --dataset {YOUR_DATASET_HERE}`

Example:
```bash
:~$ snapper --dataset devdataset
[2026-08-03 14:33:28.746] [snapper] [info] Created snapshot: devdataset@2026-08-03T12:33Z
:~$ zfs list -t snapshot
NAME                                  USED  AVAIL  REFER  MOUNTPOINT
devdataset@2026-08-03T12:33Z            0B      -    24K  -
```

## systemd setup:
I feel like a good default is to snapshot the datasets shortly after boot and then once every 12 hours.

### Example: snapper-devdataset.service
```bash
[Unit]
Description=Snapshot devdataset

[Service]
Type=oneshot
ExecStart=/usr/local/bin/snapper --dataset devdataset
User=vemund
Group=vemund
```

### Example: snapper-devdataset.timer
```bash
[Unit]
Description=Periodically run snapper-devdataset

[Timer]
OnBootSec=1min
OnUnitActiveSec=12h

[Install]
WantedBy=timers.target
```
