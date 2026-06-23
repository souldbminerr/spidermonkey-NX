let x = 40, y = 22;
for (;;) {
  const h = nx.buttonsHeld();
  if (h & nx.B) break;
  if (h & nx.Left) x--;
  if (h & nx.Right) x++;
  if (h & nx.Up) y--;
  if (h & nx.Down) y++;
  if (x < 2) x = 2; if (x > 78) x = 78;
  if (y < 3) y = 3; if (y > 44) y = 44;
  nx.clear();
  nx.drawAt(2, 1, 'mover.js  -  move O with the D-Pad, B to go back');
  nx.drawAt(x, y, 'O');
  nx.present();
  nx.sleep(16);
}
