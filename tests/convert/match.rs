fn main() {
    let n = 3;
    match n {
        1 => println!("one"),
        2 | 3 => {
            println!("two or three");
        }
        _ => println!("other"),
    }
}
