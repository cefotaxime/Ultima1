# M255 hotfix1 — legacy audit compatibility

The first M255 package added `!mondainIntroBlocking` directly to several existing HUD/Ready rendering conditions. Runtime behaviour was correct, but historical source-contract audits such as M97 compare those expressions literally and therefore aborted before the build.

Hotfix1 keeps the M255 Time Machine page unchanged and moves the Mondain intro condition to one outer presentation guard. Inside that guard the historical Ready predicates remain verbatim:

```cpp
if(!space.active) drawReadyPopup(r,font,commands,p);
if(commands.active()) drawReadyCommandInput(r,font,original,commands);
```

The packaged M97 audit is also forward-compatible with an equivalent outer guard, and the M255 audit checks the outer blocking guard rather than the superseded direct-expression form.

No gameplay, translation, sound, timing, save-format, or combat rule is changed.
