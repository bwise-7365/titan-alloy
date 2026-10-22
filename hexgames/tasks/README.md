Copyright Ben Paul Wise. All Rights Reserved.

# tasks/ — worker state files

One file per delegated task, `NN-slug.md`, owned by its worker and merged into `PLAN.md` by the
coordinator. The `resume:` line is rewritten after every green build; after a crash the new session
continues from it. Template:

```
Copyright Ben Paul Wise. All Rights Reserved.
# Task NN: <title>
status: assigned            # assigned | in-progress | blocked | review | done
worker: W3 (sonnet)         started: 2026-09-14
resume: <one line: what is done, what is next, last green ctest command>
inputs: <exact files the worker reads; nothing else>
outputs: <files the worker writes>
acceptance: <ctest labels that must be green; goldens that must be unchanged>
log:
- 2026-09-14 10:10 created
Copyright Ben Paul Wise. All Rights Reserved.
```

Workers report in this file, not in chat prose, and do not explore the repository beyond `inputs`.

Copyright Ben Paul Wise. All Rights Reserved.
