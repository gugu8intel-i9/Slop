func greet(name: String) -> String {
    return "Hello, \(name)!"
}

let names = ["Ada", "Grace"]
for n in names {
    print(greet(name: n))
}
var total = 0
for i in 1...3 {
    total += i
}
print("total \(total)")
