const COLORS = [
  ['red',     '\x1b[31m'],
  ['green',   '\x1b[32m'],
  ['yellow',  '\x1b[33m'],
  ['blue',    '\x1b[34m'],
  ['magenta', '\x1b[35m'],
  ['cyan',    '\x1b[36m'],
  ['white',   '\x1b[37m'],
];
const RESET = '\x1b[0m';

let x = 40, y = 22, ci = 0;
const dots = [];
let prev = nx.buttonsHeld();
for (;;) {
  const h = nx.buttonsHeld();
  const hit = h & ~prev;
  prev = h;

  if (h & nx.B) break;
  if (hit & nx.Left) x--;
  if (hit & nx.Right) x++;
  if (hit & nx.Up) y--;
  if (hit & nx.Down) y++;
  if (x < 2) x = 2; if (x > 78) x = 78;
  if (y < 3) y = 3; if (y > 44) y = 44;
  if (hit & nx.L) ci = (ci + COLORS.length - 1) % COLORS.length;
  if (hit & nx.R) ci = (ci + 1) % COLORS.length;
  if (h & nx.A) dots.push([x, y, ci]);
  if (h & nx.Y) dots.length = 0;

  nx.clear();
  nx.drawAt(2, 1, 'paint.js  -  D-Pad move, A draw, L/R color, Y clear, B back');
  nx.drawAt(2, 2, 'Color: ' + COLORS[ci][1] + COLORS[ci][0] + RESET);
  for (const d of dots) nx.drawAt(d[0], d[1], COLORS[d[2]][1] + '#' + RESET);
  nx.drawAt(x, y, COLORS[ci][1] + '+' + RESET);
  nx.present();
  nx.sleep(16);
}
