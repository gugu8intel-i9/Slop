using System;

class Program {
    static int Twice(int x) {
        return x * 2;
    }

    static void Main(string[] args) {
        string name = "Slop";
        int n = Twice(21);
        Console.WriteLine($"Hello {name}, {n}");
        Console.WriteLine("{0} and {1}", n, name);
    }
}
