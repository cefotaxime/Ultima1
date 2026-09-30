# M259 hotfix2 — final audit marker compatibility

M259 hotfix1 changed the launcher's final audit marker to `All audits through M259 hotfix1 OK`. The historical M258 startup audit intentionally parses the canonical numeric form `All audits through M<N> OK`, so that textual suffix caused the regex to return no match and aborted the launcher before CMake.

Hotfix2 restores the canonical marker:

```text
[audit] All audits through M259 OK
```

The package revision is printed separately:

```text
[audit] Package revision: M259 hotfix2
```

A dedicated regression audit now pins this contract. The distributable also contains no `build/` tree or `CMakeCache.txt`.
