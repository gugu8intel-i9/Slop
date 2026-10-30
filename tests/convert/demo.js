// greets people
function greet(name) {
    return `Hello ${name}!`;
}

const people = ['Ada', 'Linus'];
people.push("Grace");
for (const p of people) {
    console.log(greet(p));
}
let count = 0;
for (let i = 0; i < people.length; i++) {
    if (people[i].startsWith("G") || people[i] === "Ada") {
        count++;
    }
}
console.log("matches:", count);
