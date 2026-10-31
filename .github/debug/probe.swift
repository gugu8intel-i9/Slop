import Metal
let dev = MTLCreateSystemDefaultDevice()!
print("device:", dev.name, "apple7:", dev.supportsFamily(.apple7), "mac2:", dev.supportsFamily(.mac2))
let tests: [(String, String)] = [
 ("float", "kernel void k(device float* a [[buffer(0)]], uint g [[thread_position_in_grid]]) { a[g] = a[g] * 2.0f; }"),
 ("int", "kernel void k(device int* a [[buffer(0)]], uint g [[thread_position_in_grid]]) { a[g] = a[g] * 2; }"),
 ("long", "kernel void k(device long* a [[buffer(0)]], uint g [[thread_position_in_grid]]) { a[g] = a[g] * 2; }"),
 ("long-div", "kernel void k(device long* a [[buffer(0)]], uint g [[thread_position_in_grid]]) { a[g] = a[g] / 3 + a[g] % 7; }"),
 ("constant-long", "kernel void k(device int* a [[buffer(0)]], constant long& n [[buffer(1)]], uint g [[thread_position_in_grid]]) { if ((long)g < n) a[g] = 1; }"),
 ("atomic", "kernel void k(device atomic_int* e [[buffer(0)]], uint g [[thread_position_in_grid]]) { atomic_store_explicit(e, 1, memory_order_relaxed); }"),
 ("template", "template<typename T> T rd(device T* a, int i) { return a[i]; }\nkernel void k(device float* a [[buffer(0)]], uint g [[thread_position_in_grid]]) { a[g] = rd(a, (int)g); }"),
 ("long-to-float", "kernel void k(device float* a [[buffer(0)]], device long* b [[buffer(1)]], uint g [[thread_position_in_grid]]) { a[g] = float(b[g]); b[g] = long(a[g]); }"),
]
for (name, body) in tests {
  let src = "#include <metal_stdlib>\nusing namespace metal;\n" + body
  do { _ = try dev.makeLibrary(source: src, options: nil); print("OK  ", name) }
  catch { print("FAIL", name, error) }
}
for f in CommandLine.arguments.dropFirst() {
  let src = try! String(contentsOfFile: f, encoding: .utf8)
  do { _ = try dev.makeLibrary(source: src, options: nil); print("OK  ", f) }
  catch { print("FAIL", f, error) }
}
