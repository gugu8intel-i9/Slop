class Dog:
    def __init__(self, name):
        self.name = name

def main():
    total = 0
    for i in range(1, 4):
        total += i * 2.5
    print("total", total)

main()
