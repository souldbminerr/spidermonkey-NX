const fb = nx.gfx.init();
let t = 0;
try {
  for (;;) {
    if (nx.buttonsHeld() & nx.B) break;
    nx.gfx.clear(16, 16, 24);
    for (let i = 0; i < 8; i++) {
      const x = (Math.sin(t / 30 + i) * 0.5 + 0.5) * (fb.width - 120);
      const y = (Math.cos(t / 37 + i * 1.7) * 0.5 + 0.5) * (fb.height - 120);
      nx.gfx.fillRect(x, y, 120, 120, (i * 32) & 255, (255 - i * 24) & 255, 128, 180);
    }
    nx.gfx.text("Press B to exit", 40, 40, 255, 255, 255, 255, 3);
    nx.gfx.text("abcdefghijklmnopqrstuvwxyz 0123456789", 40, 90, 120, 220, 255, 255, 2);
    const tp = nx.touch();
    if (tp.count > 0) {
      nx.gfx.fillRect(tp.x - 20, tp.y - 2, 40, 4, 255, 255, 255, 255);
      nx.gfx.fillRect(tp.x - 2, tp.y - 20, 4, 40, 255, 255, 255, 255);
    }
    nx.gfx.present();
    nx.sleep(16);
    t++;
  }
} finally {
  nx.gfx.deinit();
}
