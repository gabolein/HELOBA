Authors: Gabriel Behrendt, Jonas Kuß

# HELOBA

HELOBA is a heap-based load-balancer for wireless communication. It was developed for a [Beagle Bone Black](https://www.beagleboard.org/boards/beaglebone-black) equipped with a [CC1200](https://www.ti.com/product/CC1200?&bm-verify=AAQAAAAJ_____yPQuMhfG5I2FLqLBwOJjS6QFbmhrJ70vDhQTg8JyXs9wHozsX5O4oXk5ZwHneE3tsNo7fyjfHJzOQo3Bj5P07MhWrcbHq-LF3tlLZKKtJTlEsD-VNak2Flcx5-vCR5hRv_GDyrc8ITfPPzVivv5OCSVihZKRZj_5Mp6bhliY6LGcVqmzlk6mZooP9OjGXHixzFiu7ONYhJXRe53ER_x0dwdm5v7EgTVE9PRhwUJB_jQWAJZ9L12ZYFfJd-Mk_N9AY5sZB8Wo04YvyOWkZZzX1Jb9Homk4b9xhM8Osvi) and running Debian. However, we have fully virtualized the communication aspect, meaning you can run multiple nodes locally in separate terminals.

If you wish to know more about motivation, design decisions and evaluation (and can read German), feel free to check out our [report](report.pdf).

## Installation

Targets:
- `make release` builds the project.
- `make debug` builds the project with debug symbols, debug output and ASAN.
- `make simulation` builds the project without debug symbols, but with debug output in order to collect statistics.
- `make run` runs the program.
- `make test` runs the test suite. Requires [criterion](https://github.com/Snaipe/Criterion).
- `make clean` deletes the build files.
- `make deploy` copies the source code onto the target device.

In case you want to run the code on a Beaglebone, you need to remove the `-DVIRTUAL` flag from `Makefile:3`.

## Demo

Run the following commands:
1. Open four separate terminals
2. Start the program in each of these terminals.
3. Run the `freq` and `id` command for each node. One node will show as leader. Run the `list` command on the leader.
4. Run the `split` command on the leader. Repeat step 3.
5. Use the `searchfor` command to search for a node on a different frequency.
