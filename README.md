## Sherlock

### An investigative LV2 plugin bundle

This plugin bundle contains plugins for visualizing LV2 atom, MIDI and
OSC events.

Use them for monitoring and debugging of event signal flows inside plugin graphs.

#### Build status

[![build status](https://gitlab.com/OpenMusicKontrollers/sherlock.lv2/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/sherlock.lv2/commits/master)

### Binaries

For GNU/Linux (64-bit, 32-bit, armv7), Windows (64-bit, 32-bit) and MacOS
(64/32-bit univeral).

To install the plugin bundle on your system, simply copy the __sherlock.lv2__
folder out of the platform folder of the downloaded package into your
[LV2 path](http://lv2plug.in/pages/filesystem-hierarchy-standard.html).

#### Stable release

* [sherlock.lv2-0.24.0.zip](https://dl.open-music-kontrollers.ch/sherlock.lv2/stable/sherlock.lv2-0.24.0.zip) ([sig](https://dl.open-music-kontrollers.ch/sherlock.lv2/stable/sherlock.lv2-0.24.0.zip.sig))

#### Unstable (nightly) release

* [sherlock.lv2-latest-unstable.zip](https://dl.open-music-kontrollers.ch/sherlock.lv2/unstable/sherlock.lv2-latest-unstable.zip) ([sig](https://dl.open-music-kontrollers.ch/sherlock.lv2/unstable/sherlock.lv2-latest-unstable.zip.sig))

### Sources

#### Stable release

* [sherlock.lv2-0.24.0.tar.xz](https://git.open-music-kontrollers.ch/lv2/sherlock.lv2/snapshot/sherlock.lv2-0.24.0.tar.xz)
([sig](https://git.open-music-kontrollers.ch/lv2/sherlock.lv2/snapshot/sherlock.lv2-0.24.0.tar.xz.asc))

#### Git repository

* <https://git.open-music-kontrollers.ch/lv2/sherlock.lv2>

### Packages

* [ArchLinux](https://www.archlinux.org/packages/community/x86_64/sherlock.lv2/)

### Bugs and feature requests

* [Gitlab](https://gitlab.com/OpenMusicKontrollers/sherlock.lv2)
* [Github](https://github.com/OpenMusicKontrollers/sherlock.lv2)

### Plugins

#### Atom Inspector

![Sherlock Atom Inspector](/screenshots/screenshot_1.png)

The _Atom Inspector_ is meant as a monitor/debug tool for LV2 plugin
and host authors. It captures all Atom events sent to its event input port
and presents them on its user interface for convenient nested browsing.

#### MIDI Inspector

![Sherlock MIDI Inspector](/screenshots/screenshot_2.png)

The _MIDI Inspector_ is meant as a monitor/debug tool for LV2 plugin
and host authors. It captures all MIDI events sent to its event input port
and presents them on its user interface for convenient nested browsing.

#### OSC Inspector

![Sherlock OSC Inspector](/screenshots/screenshot_3.png)

The _OSC Inspector_ is meant as a monitor/debug tool for LV2 plugin
and host authors. It captures all OSC events sent to its event input port
and presents them on its user interface for convenient nested browsing.

#### License

Copyright (c) 2015-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)

This is free software: you can redistribute it and/or modify
it under the terms of the Artistic License 2.0 as published by
The Perl Foundation.

This source is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
Artistic License 2.0 for more details.

You should have received a copy of the Artistic License 2.0
along the source as a COPYING file. If not, obtain it from
<http://www.perlfoundation.org/artistic_license_2_0>.
