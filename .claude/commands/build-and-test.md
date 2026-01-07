# Build and Test

Execute the full build and test pipeline for the MMORPG server project.

## Steps

1. **Configure CMake** (if build directory doesn't exist)
   ```bash
   cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
   ```

2. **Build all targets**
   ```bash
   cmake --build build
   ```

3. **Run unit tests**
   ```bash
   ctest --test-dir build --output-on-failure
   ```

4. **Report results**
   - Show build status (success/failure)
   - Show test summary (passed/failed/total)
   - If any failures, show error details

## On Failure

- If build fails: Show compiler errors and suggest fixes
- If tests fail: Show failing test names and assertions
