# metavirt

## Building metavirt

metavirt requires LLVM version 12 and CMake version >= 3.20. Use CMake presets `develop` or `release`
to build.

### 2.1 Build example

metavirt uses CMake to build. Example build recipe (release build, installs to default prefix
`${metavirt_SOURCE_DIR}/install/metavirt`)

```sh
$> cd metavirt
$> cmake --preset release
$> cmake --build build --target install --parallel
```
