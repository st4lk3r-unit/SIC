# Troubleshooting

**No serial logs** — match baud, avoid long blocking in setup.
**Duplicate symbols** — `-DSIC_DISABLE_DUMMY=1`.
**Mic silent** — sample rate/channels; ensure 16k→48k upsampler if needed.
**Keyboard dead** — wiring + pull-ups; adjust debounce/scan period.
