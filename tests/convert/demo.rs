use std::io;

struct Point {
    x: i32,
    y: i32,
}

impl Point {
    fn new(x: i32, y: i32) -> Self {
        Point { x: x, y: y }
    }

    fn sum(&self) -> i32 {
        self.x + self.y
    }
}

/* adds two numbers */
fn add(a: i32, b: i32) -> i32 {
    a + b
}

fn main() {
    let name = String::from("Slop");
    let mut total: i32 = 0;
    for i in 0..=4 {
        total += i;
    }
    let nums = vec![3, 1, 2];
    println!("Hi {}, total {} and {} numbers", name, total, nums.len());
    let p = Point::new(2, 5);
    println!("sum = {}", p.sum());
    if total > 5 && !name.is_empty() {
        println!("big {}", add(total, 1));
    } else {
        println!("small");
    }
    let half = total as f64 / 4.0;
    println!("{name} has {half}");
}
