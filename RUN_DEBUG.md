# Running Hyperterm - Debug Info

The program might crash because rendering isn't fully implemented yet. If you see errors:

1. **Vulkan cleanup errors**: Fixed - handles are now properly initialized and checked
2. **Runtime crashes**: Expected - rendering functions are placeholders
3. **No display**: Expected - vertex buffers not implemented yet

To run with error output:
```bash
cd build
./Hyperterm 2>&1 | tee output.log
```

Or run in gdb for debugging:
```bash
cd build
gdb ./Hyperterm
(gdb) run
(gdb) bt  # backtrace if it crashes
```
