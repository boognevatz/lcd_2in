# Three-Bucket Scenario Verification Action Plan

## Overview
This action plan describes a methodical verification process for Scenario 6
in the three-bucket camera-to-TX system.
All rules and the scenario table are in `three_bucket_system.md`.
When in doubt, Scenario 1,2,3,4,5 can be used for refence, it was already debugged and 
can be safely considered error-free.

## IMPORTANT

Always execute all subagents in parallel!
Always run subagents on only one row at a time! So each report can correspond
to only one row. The main agent saves each report to the file. It is
the job of @opus-judge, if there is not decision making necessary, the 
file saving job is still his. If there are more work needed (as described below), 
then those also.
The table rows are always numbered starting from 0, the first column (tick0, tick1, etc), 
can be directly see as the row number also.

### Mandatory Subagent Prompt Preamble
Every subagent dispatch (worker, judge, or fallback) MUST include the following
preamble at the top of its prompt, verbatim:

> **ROW NUMBERING RULE:** Table rows are numbered starting from 0.
> The first column in the table (tick0, tick1, tick2, …) doubles as the row number.
> For example, the row whose first column reads "tick 0" is row 0,
> "tick 1" is row 1, and so on. Use this zero-based numbering in all
> references, filenames, and reasoning.

Do NOT omit or paraphrase this preamble — paste it unchanged into every prompt.

## Worker Agents
- `@big-pickle` - Large context reasoning, complex multi-step logic
- `@trinity` - Code generation, technical deep-dives
- `@minimax` - Fast summarization, structured extraction

## Judge Agent
- `@opus-judge` - Arbitrates when errors are found and cross-checked

## Fallback Agent
- `@opus-fallback` - Invoked only if `@opus-judge` flags all outputs as low quality

## Definition of "Correct"
A row is correct if it needs NO changes — the file is assumed correct as-is. 
The burden of proof is on any ERROR claim. Subagents validate each row against the rules and the previous row's state.

## File Naming Convention
- OK: `./analyzed/three_scenario6_row{N}_{agent}_OK.md`
- ERROR: `./analyzed/three_scenario6_row{N}_{agent}_ERROR.md`
- Cross-check: `./analyzed/three_scenario6_row{N}_{agent}_ERROR_{crosscheck_agent}_OK.md` (or `_ERROR.md`)

## Verification Protocol


There is a file named @three_bucket_system_diff.md , which details what 
row may be faulty in the current @three_bucket_system.md.

Your job is to verify and report if there is an error.
Use the subagent system for verification for each line.

### Step 1: Initial Row Verification
For each row N :
1. Dispatch all three worker agents to verify row N against the rules in `three_bucket_system.md`
2. Each agent reads `three_bucket_system.md` directly
3. Each agent returns a verdict: OK or ERROR with reasoning

### Step 2: Error Cross-Checking
If any agent flags ERROR:
1. For each ERROR report, feed it to the other two agents for cross-checking
2. Save cross-check reports with compound filenames
3. Collect all reports (initial + cross-checks) for row N

### Step 3: Judge Decision
1. Send all reports for row N to `@opus-judge`
2. Judge evaluates:
   - If error is invalid (cross-checkers agree row is correct) → row stays unchanged
   - If error is valid → judge picks or synthesizes best correction
3. Apply correction if needed before proceeding to next row

### Step 4: Sequential Progression
- Each row is verified in order 
- If a row is corrected, the fix is applied before moving to the next row
- Each row's state depends on all previous rows being correct

## Report File Requirements
Each report file must contain, in this order:

1. **Prompt (verbatim):** The exact, unedited prompt that was sent to the subagent,
   enclosed in a fenced block (` ```prompt ... ``` `). This includes the mandatory
   ROW NUMBERING RULE preamble and the full task description.
2. **Thinking / Reasoning:** The subagent's complete chain-of-thought — every
   intermediate step, rule application, and deduction it performed. Nothing may
   be summarised or omitted; include the full thinking as returned by the agent.
3. **Verdict:** OK or ERROR, with a concise final summary.
4. **Correction (if ERROR):** The proposed fix and justification.

This structure ensures any report can be independently audited and re-run
without needing to reconstruct what the subagent was asked or how it reasoned.

## Final Validation Pass
After all 19 rows are processed:
1. Run one full-table end-to-end verification with `@big-pickle`
2. Confirm internal consistency across all rows
3. Document any remaining issues

## Quality Assurance
- Burden of proof is on ERROR claims
- Cross-checking ensures robust validation
- Sequential correction maintains dependency integrity
- Final pass catches any cascading issues

## Execution Notes
- All agents read from the same source: `three_bucket_system.md`
- No separate rule extraction step needed
- Judge only invoked when errors are found
- Fallback only if judge determines all outputs are low quality
- Always execute all subagents in parallel.

