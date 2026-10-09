# HF84 — First launch Shift-H help

Based on HF83. After character creation succeeds, create a per-slot `PLAYERn.U1.krnewcharacter` marker and remove any prior `.krhelpseen` marker. On the first `FrontendAction::StartGame` for that slot, check for the new-character marker and absence of a seen marker, then run the existing command-help popup (`log.pushCommand("커맨드 도움말")`, `commandHelpPopup=true`, `commandHelpPresented=false`). Mark as seen and remove the pending marker. Subsequent starts of that character do not automatically display the help. Older character saves do not get an automatic popup. The original DOS save format remains unchanged.

Windows MinGW build passed. Package on connected PC: `Ultima1_KR_Work/packages/HF84_FIRST_LAUNCH_HELP.zip`. Interactive UI test remains pending.
