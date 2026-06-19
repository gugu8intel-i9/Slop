fn main() {
    // Tell cargo to compile our runtime C file and link it directly
    cc::Build::new()
        .file("../slop_rt.c")
        .include("..")
        .compile("slop_rt");

    // Tell cargo to invalidate the built crate if the runtime source changes
    println!("cargo:rerun-if-changed=../slop_rt.c");
    println!("cargo:rerun-if-changed=../slop_rt.h");
}
