# TODO (Release Blockers / Nice-to-haves)

**Blockers for 0.3.0 final:**
- Replace dummy IR with real RMT-based `ir_tx` (ESP-IDF RMT or Arduino wrapper).
- Provide SD (SPI) minimal driver wired to Arduino `SD` lib or `sdmmc_host`.
- Add `autoreg.c` that safely links only compiled drivers (guard with `#if`).

**Nice-to-have:**
- Charger TP4057: add ADC-based VBAT & charge current sampling.
- Unit tests for registry with multiple drivers per function.
- CI: PlatformIO build for both envs.
