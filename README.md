# LibModuleManager
a forwards &amp; module manager for SourceMod.

## Purpose
To reduce the boilerplate needed to setup and create forwards as well as Plugin management systems.

## Features
* Global Forwards Manager - controlled and managed via Config file.
* Private Forward Managers - Any plugin using libmodulemanager can request one or more private forward managers, each individually controlled by a config file.
* Module/Plugin Managers - Any plugin using libmodulemanager, just like the Private Forward Managers, can request one or more Plugin Managers.

Each type of manager has different strengths to them.
