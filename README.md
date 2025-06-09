## hycov
A Hyprland overview mode plugin.

The [original repository](https://github.com/DreamMaoMao/hycov) was archived and hard to update to current version of hyprland API. <br>
This is the reimplementation of it. <br>
It has less features for now but a more stable and easy to scale codebase to maintain. <br>

## Roadmap
  - add several new modes (overview workspace, overview monitor, overview all)
  - add workspaces in overview
  - restore some old features from the original repo

## Config
For now it has only one dispatcher that toggle the overview.
```
bind = SUPER, overview:toggle
```

## Gallery
https://github.com/DreamMaoMao/hycov/assets/30348075/59121362-21a8-4143-be95-72ce79ee8e95

## Installation
I tested only manual installation for now:
  - build with cmake and gcc
  - then ```hyprctl plugin load full_path_to.so```
