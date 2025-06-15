## hycov
A Hyprland overview mode plugin.

The [original repository](https://github.com/DreamMaoMao/hycov) was archived and hard to update to current version of hyprland API. <br>
This is the reimplementation of it, that has less features for now but a more stable, easy to maintain and scale codebase. <br>

## Features
  - Preserves prev layout on return from overview
  - Several overview modes - all, workspace, monitor
  - hot corner has separete mappings to mods |----'monitor'----x----'workspace'----x----'all'----|
  - Scales windows and their framebuffers properly according to the previous ratio
  - Does not passes key and resize events to windows

## Config
Example config
```
bind = $mainMod, TAB, overview:enter, monitor
bind = $mainMod, RETURN, overview:enter, all
bind = $mainMod, GRAVE, overview:enter, workspace

# binds submap that will automatically activate in overview
submap = overview
	# same shortcut as in global submap wont work
	bind = , ESCAPE, overview:leave
	bind = , RETURN, overview:leave

	# move focus around
	bind = , LEFT, overview:left
	bind = , RIGHT, overview:right
	bind = , UP, overview:up
	bind = , DOWN, overview:down

	bind = , a, overview:left
	bind = , d, overview:right
	bind = , w, overview:up
	bind = , s, overview:down
submap = reset

```

## Gallery
https://github.com/DreamMaoMao/hycov/assets/30348075/59121362-21a8-4143-be95-72ce79ee8e95

## Installation
I tested only manual installation for now:
  - build with cmake and gcc
  - then ```hyprctl plugin load full_path_to.so```
You could also just download precompiled library.


## Known issues 
  - bugs in workspace mode with empty workspace
  - some parameters are not exposed to hyprland config
  - not tested with window groups
  - currently only for selecting window, closing and opening windows is not support
  - cannot set same bind in 'reset' and 'overview' submaps

## Roadmap
  - add workspaces in overview
  - add dispatcher to enter fullscreen on window selection
  - add 'hide window' functionality
