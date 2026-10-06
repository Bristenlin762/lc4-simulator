# Recording the First Demo

Record a terminal window with a readable font. Build before recording so the video can focus on the program's behavior.

```sh
make
./lc4_debugger examples/sum.hex
```

## Suggested 45–60 second sequence

1. Show `examples/sum.hex` briefly: the program adds 5 through 1, stores 15 at x2000, and reads it back. Explain that the comments show assembly equivalents; this file itself contains machine-code words.
2. Enter `regs` and `mem 0000 10` to show initial CPU state and loaded instructions.
3. Enter `step` five times. The first four instructions initialize the registers; the fifth performs the first addition. R0 becomes x0005.
4. Enter `run 16` to finish the remaining loop iterations and the store/load.
5. Enter `mem 2000 1`: x000F is decimal 15. R0 and R3 also contain x000F.
6. Enter `quit`.

Suggested captions:

- “Load machine code into a 16-bit LC4 simulator”
- “Step through instructions and inspect CPU state”
- “Execute a loop with an explicit instruction budget”
- “Verify the result in registers and memory: 15”

Do not describe this recording as verification of every instruction or as a complete PennSim replacement. This is an instruction execution and debugging demo.

## Add the video to GitHub

Place an optional GIF at `assets/demo.gif`, then add a linked image to README:

```markdown
[![LC4 simulator demo](assets/demo.gif)](YOUR_VIDEO_URL)
```

For a smaller GIF, use:

```html
<a href="YOUR_VIDEO_URL">
  <img src="assets/demo.gif" width="650" alt="LC4 simulator demo">
</a>
```

Replace `YOUR_VIDEO_URL` with the real uploaded video link. Keep generated binaries out of the repository; the Makefile rebuilds them.
