public class Demo {
    static int fib(int n) {
        if (n < 2) {
            return n;
        }
        return fib(n - 1) + fib(n - 2);
    }

    public static void main(String[] args) {
        int[] nums = {5, 7, 9};
        String who = "Slop";
        int total = 0;
        for (int n : nums) {
            total += n;
        }
        System.out.println("Hello " + who + " " + total);
        System.out.printf("fib(10) = %d%n", fib(10));
        for (int i = 3; i >= 1; i--) {
            System.out.println(i);
        }
    }
}
