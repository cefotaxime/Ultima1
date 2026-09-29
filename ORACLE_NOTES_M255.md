# M255 — Time Machine first-page Korean presentation

M255 restores the first blocking Time Machine narration page from `MONDAIN.EXE` and applies the user-approved Korean wording without changing the source page's nine-row structure.

`MONDAIN.EXE` raw `0x1D9B..0x1EAE` prints the nine source text rows at BIOS `(row,col)` positions `(3,5)`, `(4,5)`, `(6,12)`, `(9,4)`, `(11,5)`, `(12,11)`, `(14,4)`, `(15,6)`, `(16,6)`. The `R    G    B    W` row is replaced, not supplemented, by the one Korean colour-label row `붉은색 / 녹색 / 푸른색 / 흰색`, so the narration remains nine rows.

The four original colour anchors remain columns `12,17,22,27`, and the four source diamond/gem marks remain on row 7. Korean labels are centred on those source cells; consequently the second syllable `은` of `붉은색` begins exactly at the original `R` column. This is pixel-positioned rather than implemented with guessed spaces.

The approved Korean narration is:

```text
기체에 들어선 당신은
표시된 네 개의 홈을 발견합니다.
붉은색 / 녹색 / 푸른색 / 흰색   (source RGBW anchors retained)
알맞은 보석을 각 홈에 끼워 넣습니다.
좀 더 살펴본 당신은
두 가지 사실을 알게 됩니다.
'발사'라고 적힌 버튼이 있고,
당신은 기체 안에 갇혀 있어
버튼을 누르는 수밖에 없습니다!
```

`locked in` is intentionally rendered as `기체 안에 갇혀 있어`; it is not weakened to a mere locked-door description.

After the page, source routine raw `0x3CE2` waits for Return (`0x0D`) or Space (`0x20`). M255 now gives this page exclusive screen ownership and suppresses the normal frame, status HUD, command log, and Mondain AI until Space/Return is pressed.

The package's historical M23 oracle audit is extended to pin all nine original strings, their source row/column call sites, and the Return/Space wait predicate. M254 death/resurrection timing, M253 HyperJump blocking-row behavior, M252 HyperJump timing, and M246 audio transport remain unchanged.
