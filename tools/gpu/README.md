# GPU kernels kept as source

`matmul.comp` is the tuned matrix multiply used by `gpu_matmul` through Vulkan.
Its SPIR-V is embedded in `runtime/vulkan.slop` (`__vk_mm_spv`). To rebuild it:

```
glslangValidator -V --target-env vulkan1.0 -o mm.spv matmul.comp
spirv-opt --strip-debug -O mm.spv -o mm2.spv
spirv-val --target-env vulkan1.0 mm2.spv
```

then paste the bytes of `mm2.spv` as `\xHH` escapes into `__vk_mm_spv`. The
OpenCL (`runtime/opencl.slop`) and Metal (`runtime/macos.slop`) versions are
the same algorithm written in those languages.
