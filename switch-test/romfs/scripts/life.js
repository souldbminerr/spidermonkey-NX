const W = 78, H = 41, X0 = 2, Y0 = 4;

function seed() {
  const g = [];
  for (let y = 0; y < H; y++) {
    g[y] = [];
    for (let x = 0; x < W; x++) g[y][x] = Math.random() < 0.25 ? 1 : 0;
  }
  return g;
}

function step(g) {
  const next = [];
  for (let y = 0; y < H; y++) {
    next[y] = [];
    for (let x = 0; x < W; x++) {
      let n = 0;
      for (let dy = -1; dy <= 1; dy++)
        for (let dx = -1; dx <= 1; dx++) {
          if (dx === 0 && dy === 0) continue;
          n += g[(y + dy + H) % H][(x + dx + W) % W];
        }
      next[y][x] = (g[y][x] && (n === 2 || n === 3)) || (!g[y][x] && n === 3) ? 1 : 0;
    }
  }
  return next;
}

let grid = seed();
for (;;) {
  const h = nx.buttonsHeld();
  if (h & nx.B) break;
  if (h & nx.A) grid = seed();
  nx.clear();
  nx.drawAt(2, 1, "life.js  -  Conway's Game of Life, A to reseed, B to go back");
  for (let y = 0; y < H; y++) {
    let line = '';
    for (let x = 0; x < W; x++) line += grid[y][x] ? 'O' : ' ';
    nx.drawAt(X0, Y0 + y, line);
  }
  nx.present();
  grid = step(grid);
  nx.sleep(60);
}
