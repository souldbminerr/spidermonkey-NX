nx.clear();
console.log('Hello from hello.js');
console.log('20 + 22 =', 20 + 22);
console.log('Math.sqrt(144) =', Math.sqrt(144));
let s = 0;
for (let i = 1; i <= 100; i++) s += i;
console.log('sum 1..100 =', s);
function fact(n) { return n <= 1 ? 1 : n * fact(n - 1); }
console.log('fact(10) =', fact(10));
console.log('unix time =', nx.time());
console.log('');
console.log('Press B to go back');
nx.present();
while (!(nx.buttonsHeld() & nx.B)) nx.sleep(16);
