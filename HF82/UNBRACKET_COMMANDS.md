# HF82 — remove brackets from command labels

Relative to `HF81/src/main.cpp`, change only these lines:

```diff
- log.pushCommand("[커맨드 도움말]");
+ log.pushCommand("커맨드 도움말");
- log.pushCommand("[버전 정보]");
+ log.pushCommand("버전 정보");
```

`Log::pushCommand` draws the `>` marker itself, so these become `>커맨드 도움말` and `>버전 정보`. Windows build succeeded. Local package: `Ultima1_KR_Work/packages/HF82_UNBRACKET_COMMANDS.zip`.
