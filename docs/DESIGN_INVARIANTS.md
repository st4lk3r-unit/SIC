# Design Invariants

- Single-include API: `<sic/sic.h>`
- No exceptions/RTTI; small heap usage.
- Feature gates via `-D` flags.
- Error codes: 0 success; negative errors.
- SIC is not an RTOS; cooperative timing.
