# Three-Bucket Scenario 2 Verification Action Plan

## Overview
This action plan describes a methodical verification process for Scenario 2
in the three-bucket camera-to-TX system. The scenario table has 19 rows (ticks
0–18). All rules and the scenario table are in `three_bucket_system.md`.
When in doubt, Scenario 1 can be used for refence, it was already debugged and 
can be safely considered error-free.

## IMPORTANT

Always execute all 4 subagents in parallel!

## Worker Agents
- `@big-pickle` - Large context reasoning, complex multi-step logic
- `@trinity` - Code generation, technical deep-dives
- `@glm5` - Balanced code and prose tasks, todo list compilation.
- `@minimax` - Fast summarization, structured extraction

## Judge Agent
- `@opus-judge` - Arbitrates when errors are found and cross-checked

## Fallback Agent
- `@opus-fallback` - Invoked only if `@opus-judge` flags all outputs as low quality

## Definition of "Correct"
A row is correct if it needs NO changes — the file is assumed correct as-is. The burden of proof is on any ERROR claim. Subagents validate each row against the rules and the previous row's state.

## File Naming Convention
- OK: `./analyzed/three_scenario2_row{N}_{agent}_OK.md`
- ERROR: `./analyzed/three_scenario2_row{N}_{agent}_ERROR.md`
- Cross-check: `./analyzed/three_scenario2_row{N}_{agent}_ERROR_{crosscheck_agent}_OK.md` (or `_ERROR.md`)

## Verification Protocol

### Step 1: Initial Row Verification
For each row N (0–18):
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
- Each row is verified in order (0 to 18)
- If a row is corrected, the fix is applied before moving to the next row
- Each row's state depends on all previous rows being correct

## Report File Requirements
Each report file must contain:
- Question: Which row, which rules were checked
- Answer: Subagent's verdict and complete reasoning
- This enables re-running the analysis independently

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

