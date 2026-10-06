# Project Instructions

## Mandatory Python Rule

1. For automation, file processing, batch operations, data scripts, image processing, and all other work that Python can perform, use directly runnable Python 3 code.
2. Do not use PowerShell, CMD, BAT, or PWSh scripts when Python can do the task.
3. Only provide PowerShell or shell scripts when the user explicitly requests `使用PowerShell` or `写shell命令`, or when Python cannot perform the required operation.
4. Do not add unsolicited PowerShell alternatives or comparisons.
5. The integrated terminal may be PowerShell, but implementation and operational logic must use Python whenever Python is capable.

## Mandatory Context Recovery

1. Built-in Codex Memories must remain enabled with both generation and use active.
2. Context Keeper must record recent user and assistant turns for this workspace.
3. Immediately after context compaction or task resume, read the Context Keeper restored snapshot before taking any other project action.
4. After restoration, reconcile saved context with the newest user request; the newest explicit request always wins.
5. When project history or prior decisions may matter, consult `C:\Users\Administrator\.codex\memories\MEMORY.md` and only open the most relevant referenced memory files.
6. Do not claim that a prior build, deployment, IDE state, board state, or runtime result is current without verifying it again.

## CJ96 Deployment Mode (Mandatory)

1. The normal deployment entry is the original FlyThingsIDE `Ctrl+Alt+R` action.
2. Keep the IDE progress window and the IDE native file-push workflow unchanged.
3. Never replace the `Ctrl+Alt+R` handler with a Python deployment script, an external async thread, or a custom JADB/ADB transfer implementation.
4. Do not patch or overwrite the FlyThingsIDE editor plugin to change deployment behavior.
5. Python deployment tools may be used only for explicit diagnostics or a separately requested manual deployment; they must not be wired into the normal IDE shortcut.
6. Do not change Windows proxy settings, network adapters, or virtual-machine network settings during deployment work.
7. Before any future deployment change, verify the original plugin and preserve a backup. If the IDE shortcut stops showing its normal progress window, restore the original plugin before further testing.

## CJ96 Icon/Image Processing Rules (Mandatory)

1. When creating or scaling UI images, always distinguish the canvas size from the actual visible icon size.
2. Before resizing, crop or measure the non-transparent, non-background content bounding box first.
3. Do not assume an icon fills its canvas; preserve the intended visible proportion of the icon inside the canvas.
4. When matching related icons, compare and align the actual visible content dimensions, not only the canvas dimensions.
5. Keep the output canvas size unchanged when the UI control expects a fixed canvas, and center the resized visible content as required.
6. Use high-quality resampling and preserve transparency/anti-aliasing; never add unwanted background or external frames unless explicitly requested.
