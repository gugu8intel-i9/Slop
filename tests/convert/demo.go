package main

import (
    "fmt"
    "strings"
)

type Point struct {
    X int
    Y int
}

func (p Point) Sum() int {
    return p.X + p.Y
}

func add(a, b int) int {
    return a + b
}

func main() {
    x := 5
    nums := []int{1, 2, 3}
    nums = append(nums, 4)
    total := 0
    for _, n := range nums {
        total += n
    }
    p := Point{X: 1, Y: 2}
    fmt.Println("total", total, "add", add(x, 2), "sum", p.Sum())
    fmt.Printf("upper: %s\n", strings.ToUpper("go"))
    for i := 0; i < 3; i++ {
        fmt.Println(i)
    }
}
