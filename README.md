## hycov
A Hyprland overview mode plugin.

The [original repository](https://github.com/DreamMaoMao/hycov) was archived and hard to update to current version of hyprland API. <br>
This is the reimplementation of it. <br>
It has less features for now but a more stable and easy to scale codebase to maintain. <br>

## Config
For now it has only one dispatcher that toggle the overview.
```
bind = $mainMod, TAB, overview:enter

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


## Known issues 
  - currently only for selecting window, closing and opening windows is not support
  - not tested with window groups
  - cannot set same bind in 'reset' and 'overview' submaps

## Roadmap
  - add several new modes (overview workspace, overview monitor, overview all)
  - add workspaces in overview
  - add dispatcher to enter fullscreen on window selection
  - add 'hide window' functionality
