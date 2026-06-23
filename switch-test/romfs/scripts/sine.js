let t = 0;
for (;;) {
  if (nx.buttonsHeld() & nx.B) break;
  nx.clear();
  nx.drawAt(2, 1, 'sine.js  -  B to go back');
  for (let col = 1; col <= 78; col++) {
    const row = 4 + Math.round(9 + 9 * Math.sin((col + t) * 0.18));
    nx.drawAt(col, row, '*');
  }
  nx.present();
  nx.sleep(33);
  t += 2;
}
