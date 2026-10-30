fun square(n: Int): Int {
    return n * n
}

fun main() {
    val name = "Slop"
    var total = 0
    for (i in 1..4) {
        total += square(i)
    }
    println("Hi $name, total ${total}")
}
