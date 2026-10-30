# a small Python program
def area(w, h):
    return w * h

def describe(name):
    return f"{name} is {len(name)} letters"

scores = [3, 9, 4]
best = 0
for s in scores:
    if s > best:
        best = s
    elif s == 4:
        print("found four")
    else:
        pass

names = []
names.append("ada")
print(describe("Slop"), area(3, 4), best)
print(", ".join(["a", "b"]), names[0].upper())
i = 0
while i < 3 and True:
    i += 1
print("done", i)
