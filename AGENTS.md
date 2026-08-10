# Project Instructions

## Mandatory Python Rule

1. For automation, file processing, batch operations, and data scripts, prefer directly runnable Python 3 code.
2. Do not default to PowerShell, CMD, BAT, or PWSh scripts.
3. Only provide PowerShell or shell scripts when the user explicitly requests `使用PowerShell` or `写shell命令`.
4. Do not add unsolicited PowerShell alternatives or comparisons.
5. The integrated terminal may be PowerShell, but implementation logic must still prefer Python.

## Mandatory Context Recovery

1. Built-in Codex Memories must remain enabled with both generation and use active.
2. Context Keeper must record recent user and assistant turns for this workspace.
3. Immediately after context compaction or task resume, read the Context Keeper restored snapshot before taking any other project action.
4. After restoration, reconcile saved context with the newest user request; the newest explicit request always wins.
5. When project history or prior decisions may matter, consult `C:\Users\Administrator\.codex\memories\MEMORY.md` and only open the most relevant referenced memory files.
6. Do not claim that a prior build, deployment, IDE state, board state, or runtime result is current without verifying it again.
