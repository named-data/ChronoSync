# ChronoSync: synchronization library for distributed realtime applications for NDN

[![CI](https://github.com/named-data/ChronoSync/actions/workflows/ci.yml/badge.svg)](https://github.com/named-data/ChronoSync/actions/workflows/ci.yml)
[![Docs](https://github.com/named-data/ChronoSync/actions/workflows/docs.yml/badge.svg)](https://github.com/named-data/ChronoSync/actions/workflows/docs.yml)
![Language](https://img.shields.io/badge/C%2B%2B-14-blue)
![Latest version](https://img.shields.io/github/v/tag/named-data/ChronoSync?label=Latest%20version)

> DEPRECATION NOTICE: ChronoSync's design is outdated. We recommend using more recent sync protocols, such as [PSync](https://named-data.net/doc/PSync/current/index.html) or [StateVectorSync](https://named-data.github.io/StateVectorSync/).

In supporting many distributed applications, such as group text messaging, file sharing,
and joint editing, a basic requirement is the efficient and robust synchronization of
knowledge about the dataset such as text messages, changes to the shared folder, or
document edits.  This library implements the
[ChronoSync protocol](https://named-data.net/wp-content/uploads/2014/03/chronosync-icnp2013.pdf),
which exploits the features of the Named Data Networking architecture to efficiently
synchronize the state of a dataset among a distributed group of users.  Using appropriate
naming rules, ChronoSync summarizes the state of a dataset in a condensed cryptographic
digest form and exchange it among the distributed parties.  Differences of the dataset can
be inferred from the digests and disseminated efficiently to all parties.  With the
complete and up-to-date knowledge of the dataset changes, applications can decide whether
or when to fetch which pieces of the data.

ChronoSync uses the [ndn-cxx](https://github.com/named-data/ndn-cxx) library.

## Installation

### Prerequisites

* [ndn-cxx and its dependencies](https://named-data.net/doc/ndn-cxx/current/INSTALL.html)

### Build

To build ChronoSync from the source:

    ./waf configure
    ./waf
    sudo ./waf install

To build on memory constrained platform, please use `./waf -j1` instead of `./waf`. The
command will disable parallel compilation.

If configured with tests (`./waf configure --with-tests`), the above commands will also
generate unit tests that can be run with `./build/unit-tests`.

## Reporting bugs

Please submit any bug reports or feature requests to the
[ChronoSync issue tracker](https://redmine.named-data.net/projects/chronosync/issues).

## Contributing

We greatly appreciate contributions to the ChronoSync code base, provided that they are
licensed under the GPL 3.0+ or a compatible license (see below).
If you are new to the NDN software community, please read the
[Contributor's Guide](https://github.com/named-data/.github/blob/master/CONTRIBUTING.md)
to get started.

## License

ChronoSync is an open source project licensed under the GPL version 3.
See [`COPYING.md`](COPYING.md) for more information.
