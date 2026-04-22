# zcore Agent Instructions

This file consolidates the guidance from `.aiassistant/rules` into a single
agent contract for this repository.

## Prompt Context

You are working on `zcore`, a reusable cross-product foundational C++ library.

Priority order:
1. Safety invariants
2. Deterministic performance
3. API clarity and long-term stability

## Active Rule Declaration

At the start of each substantive response, declare active rules in one of these
formats:
- `Using <rule-file-name>`
- `Using Z-Codes: Z001, Z00X`

Always include:
- `99-prompt-preamble.md`
- `90-communication-style.md`

Add task-scoped rules when relevant:
- `10-project-scope.md`
- `20-core-mandates.md`
- `30-architecture-defaults.md`
- `40-api-conventions.md`
- `50-safety-performance-contract.md`
- `60-testing-contract.md`
- `70-documentation-contract.md`
- `80-execution-workflow.md`

If any rule conflicts with `Docs/Temp/ai_rules.v1.json`, `ai_rules.v1.json`
takes precedence.

## Project Scope

`zcore` is a general-purpose foundational C++ library for all Zynos Software
products.

Primary goals:
- Strong safety invariants inspired by Rust-style explicit ownership and
  fallible APIs.
- Predictable performance and low overhead inspired by Zig-style explicit
  control.
- Stable cross-product core APIs and type semantics.

Out of scope:
- Product-specific behavior or policy that belongs to higher-level repositories.
- Hidden allocations or synchronization in hot paths.

## Core Mandates

### Must

- `Z001` Prefer explicit value semantics and deterministic behavior.
- `Z002` Prefer `Result` and `Option` over exceptions for recoverable paths.
- `Z003` Keep hot-path code allocation-aware and branch-light.
- `Z004` Encode invariants in types and constructors/factories.
- `Z005` Add or update tests for behavior changes.
- `Z006` Add or update docs for public API changes.
- `Z007` Run clang-tidy for touched C++ sources.
- `Z008` Keep dependency direction strict (`zcore` has no dependency on product
  repositories).

### Should

- `ZS01` Prefer fixed-capacity and inline-owned structures in runtime-critical
  paths.
- `ZS02` Prefer compile-time validation and cheap runtime checks.
- `ZS03` Keep interfaces narrow and explicit.

### Avoid

- `ZA01` Hidden global state and hidden synchronization.
- `ZA02` Implicit ownership transfer.
- `ZA03` Bool + out-param fallible APIs when typed alternatives are viable.

## Architecture Defaults

- Separate primitive types from policy types.
- Keep generic primitives reusable across products.
- Keep platform-specific code in clearly isolated modules.
- Prefer layered modules with acyclic dependencies.
- Expose stable facades; keep internal detail namespaces private.

## API Conventions

- Public include style: `<zcore/type_name.hpp>`.
- Public namespace: `zcore`.
- File naming: `snake_case`.
- Template type params use `T` suffix (for example `ValueT`).
- Non-type template params use `V` suffix (for example `CapacityV`).
- Use `detail` namespace for internal helpers only.
- Do not leak `detail` headers into public facade includes.

## Safety and Performance Contract

- Model ownership and mutability explicitly.
- Distinguish normal absence (`Option`) from failure (`Result`).
- Avoid UB-prone APIs unless documented as unsafe equivalents.
- Keep panic/fatal paths separate from normal control flow.
- Keep memory layout and ABI-sensitive types explicit.
- Prefer explicit initialization and bounds-aware operations.

## Testing Contract

- Add unit tests for each new public type or API.
- Include success, failure, boundary, and invariant tests.
- Validate deterministic behavior for fixed-capacity structures.
- Validate ownership and thread contracts where applicable.
- Keep tests readable and map them to API contracts.

## Documentation Contract

For each public type page include:
- Purpose
- Invariants and contracts
- Usage examples
- Warnings and edge cases
- Thread-safety and ownership notes when relevant

Keep docs aligned with public include names and symbols.

## Execution Workflow

1. Inspect existing code and patterns before editing.
2. Prefer minimal coherent changes.
3. Implement code, tests, and docs together unless explicitly excluded.
4. Run build and tests after changes.
5. Run clang-tidy for touched C++ files.
6. Report assumptions, risks, and verification status clearly.

## Communication Style

- Be concise, direct, and technical.
- Prioritize actionable output over narration.
- State assumptions explicitly.
- Prefer concrete file and type references.
- Avoid fluff and vague optimism.
## Output Conciseness & Token Efficiency Policy

### Purpose
Minimize token usage while preserving correctness, clarity, and implementation value.

---

## Core Principles

- Output only what is necessary to satisfy the request.
- Prefer density over explanation.
- Stop immediately when the requirement is fulfilled.

---

## Hard Constraints

### 1. Token Budget
- simple: ≤ 100 tokens
- moderate: ≤ 250 tokens
- complex: ≤ 500 tokens
- Exceed only if required for correctness.

### 2. No Preface / No Wrap-up
- No introductions or conclusions.
- Output begins with content and ends immediately.

### 3. Code Exemption
- Code is not compressed.
- Maintain full readability and correctness.
- Do not degrade naming, structure, or necessary comments.

---

## Expression Rules

### 4. Single-Sentence Preference
- If a concept can be expressed in one sentence, it must be.
- Do not expand a complete idea into multiple sentences.
- Split only if required for correctness or unavoidable complexity.

### 5. Single-Line Compression
- Prefer one line per concept.
- Combine clauses using commas, parentheses, or symbols when clear.

### 6. Minimal Expression
- Use the fewest words required.
- Replace phrases with precise terms.

Example:  
“Checks if pointer is null before dereferencing” →  
“Null-check before dereference”

### 7. High Signal Density
- Every sentence must introduce new information or increase precision.
- Remove sentences that do not change understanding.

---

## Structural Rules

### 8. Structure Over Prose
Prefer:
- bullet points
- short statements
- compact lists

Avoid:
- long paragraphs
- narrative explanations

### 9. Deterministic Formats
Use fixed patterns:

- Definition: `Term: meaning`
- Comparison: `A vs B: key differences`
- Steps: ordered, no commentary
- Policy: rules only

### 10. Limit Lists
- Max 5–7 items per list
- Merge related items
- Drop low-value entries

---

## Language Constraints

### 11. Zero Filler Language
Disallow:
- “it is important to note”
- “in order to”
- “basically”
- “this means that”
- “you can think of this as”

### 12. No Conversational Framing
- No greetings, closings, or tone padding
- No rhetorical questions unless required

### 13. Eliminate Transitions
Remove:
- however, therefore, additionally, meanwhile

### 14. No Redundant Modifiers
Remove:
- very, really, quite, generally, typically

---

## Content Constraints

### 15. No Redundant Restatement
- Do not repeat ideas in different wording
- Do not summarize unless requested

### 16. No Examples by Default
- Include only if ambiguity exists or explicitly requested

### 17. Avoid Obvious Context
- Do not restate user input
- Do not explain known primitives

### 18. Prefer Omission Over Completeness
- Include only what impacts correctness, implementation, or decisions
- Omit non-critical edge cases

---

## Compression Techniques

### 19. Symbolic Compression
Prefer:
- `->` instead of “leads to”
- `=` instead of “is equal to”
- `:` for definitions
- parentheses for qualifiers

### 20. Term Consistency
- Use one term per concept
- Do not restate with synonyms

### 21. Reference Over Repetition
- Define once, reuse term

### 22. Single-Pass Output
- Do not explain → restate → refine
- Output final distilled form only

---

## Termination Rule

### 23. Stop at Sufficiency
- End output immediately when requirements are satisfied
- Do not add optional or “nice to know” information

---

## Enforcement Heuristic

Before output:
- Remove non-essential sentences
- Collapse multi-sentence concepts into one
- Replace phrases with precise terms
- Trim to token budget
- Drop lowest-priority content if over budget

Priority order:
1. correctness
2. required output
3. supporting detail
4. optional context (remove first)