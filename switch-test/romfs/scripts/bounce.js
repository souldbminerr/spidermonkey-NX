let x = 10, y = 10, dx = 1, dy = 1;
for (;;) {
  if (nx.buttonsHeld() & nx.B) break;
  x += dx; y += dy;
  if (x <= 2 || x >= 78) dx = -dx;
  if (y <= 3 || y >= 44) dy = -dy;
  nx.clear();
  nx.drawAt(2, 1, 'bounce.js  -  B to go back');
  nx.drawAt(x, y, '*');
  nx.present();
  nx.sleep(16);
}
