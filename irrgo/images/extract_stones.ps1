Add-Type -AssemblyName System.Drawing

Add-Type -ReferencedAssemblies 'System.Drawing' -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.IO;

public static class StoneExtractor {

    static bool IsStone(byte r, byte g, byte b) {
        double lum = 0.299 * r + 0.587 * g + 0.114 * b;
        return lum < 150.0 || (r - b) > 8;
    }

    public static void Extract(string inputPath, string outputDir, int margin,
                               int nCols, int nRows) {
        var bmp = new Bitmap(inputPath);
        int W = bmp.Width, H = bmp.Height;
        Console.WriteLine("Image: " + W + "x" + H
                          + "  grid: " + nCols + "x" + nRows);

        var bd  = bmp.LockBits(new Rectangle(0,0,W,H),
                               ImageLockMode.ReadOnly,
                               PixelFormat.Format32bppArgb);
        int stride = bd.Stride;
        byte[] px  = new byte[stride * H];
        Marshal.Copy(bd.Scan0, px, 0, px.Length);
        bmp.UnlockBits(bd);

        int blackN = 0, whiteN = 0, idx = 0;

        for (int row = 0; row < nRows; row++) {
            int cellYS = row       * H / nRows;
            int cellYE = (row + 1) * H / nRows - 1;

            for (int col = 0; col < nCols; col++) {
                int cellXS = col       * W / nCols;
                int cellXE = (col + 1) * W / nCols - 1;

                // Find tight bounding box of stone pixels within this cell
                int tx1 = cellXE, ty1 = cellYE, tx2 = cellXS, ty2 = cellYS;
                bool found = false;

                for (int y = cellYS; y <= cellYE; y++) {
                    for (int x = cellXS; x <= cellXE; x++) {
                        int i = y * stride + x * 4;
                        byte b2 = px[i], g2 = px[i+1], r2 = px[i+2];
                        if (IsStone(r2, g2, b2)) {
                            if (x < tx1) tx1 = x;
                            if (x > tx2) tx2 = x;
                            if (y < ty1) ty1 = y;
                            if (y > ty2) ty2 = y;
                            found = true;
                        }
                    }
                }

                if (!found) {
                    Console.WriteLine("  [row=" + row + " col=" + col + "] NO STONE FOUND");
                    continue;
                }

                // Average color of detected stone pixels → classify
                long rSum = 0, gSum = 0, bSum = 0; int count = 0;
                for (int y = ty1; y <= ty2; y++)
                    for (int x = tx1; x <= tx2; x++) {
                        int i = y * stride + x * 4;
                        byte b2 = px[i], g2 = px[i+1], r2 = px[i+2];
                        if (IsStone(r2, g2, b2)) {
                            bSum += b2; gSum += g2; rSum += r2; count++;
                        }
                    }
                double lum = (0.299 * rSum + 0.587 * gSum + 0.114 * bSum) / count;
                string color = lum < 130 ? "black" : "white";
                int    num   = (color == "black") ? ++blackN : ++whiteN;

                // Crop with margin, clamped to image bounds
                int cx1 = Math.Max(0,   tx1 - margin);
                int cy1 = Math.Max(0,   ty1 - margin);
                int cx2 = Math.Min(W-1, tx2 + margin);
                int cy2 = Math.Min(H-1, ty2 + margin);
                int cw  = cx2 - cx1 + 1;
                int ch  = cy2 - cy1 + 1;

                // Make square so the disc is not squashed
                int side = Math.Max(cw, ch);
                int padX = (side - cw) / 2, padY = (side - ch) / 2;
                cx1 = Math.Max(0,   cx1 - padX);
                cy1 = Math.Max(0,   cy1 - padY);
                cx2 = Math.Min(W-1, cx1 + side - 1);
                cy2 = Math.Min(H-1, cy1 + side - 1);
                cw  = cx2 - cx1 + 1;
                ch  = cy2 - cy1 + 1;

                // Create output as RGBA, painting the stone and leaving
                // everything else transparent (alpha = 0)
                var outBmp = new Bitmap(cw, ch, PixelFormat.Format32bppArgb);
                var outBd  = outBmp.LockBits(new Rectangle(0,0,cw,ch),
                                             ImageLockMode.WriteOnly,
                                             PixelFormat.Format32bppArgb);
                byte[] outPx = new byte[outBd.Stride * ch];

                for (int y = cy1; y <= cy2; y++) {
                    for (int x = cx1; x <= cx2; x++) {
                        int si  = y * stride + x * 4;
                        byte b2 = px[si], g2 = px[si+1], r2 = px[si+2];
                        int ox  = x - cx1, oy = y - cy1;
                        int oi  = oy * outBd.Stride + ox * 4;
                        if (IsStone(r2, g2, b2)) {
                            outPx[oi]   = b2;   // B
                            outPx[oi+1] = g2;   // G
                            outPx[oi+2] = r2;   // R
                            outPx[oi+3] = 255;  // A = opaque
                        }
                        // else: all zeros → transparent
                    }
                }

                Marshal.Copy(outPx, 0, outBd.Scan0, outPx.Length);
                outBmp.UnlockBits(outBd);

                string outPath = Path.Combine(outputDir,
                    string.Format("{0}{1:D2}.png", color, num));
                outBmp.Save(outPath, ImageFormat.Png);
                outBmp.Dispose();

                Console.WriteLine(string.Format(
                    "  [{0,2}] {1,-12} {2,4}x{3,-4} tight=({4},{5})-({6},{7})  lum={8,5:F0}",
                    ++idx, Path.GetFileName(outPath), cw, ch,
                    tx1, ty1, tx2, ty2, lum));
            }
        }

        bmp.Dispose();
        Console.WriteLine("Done.");
    }
}
'@

[StoneExtractor]::Extract(
    "C:\repos\ghub-per\titan-alloy\irrgo\images\mixed-stones.png",
    "C:\repos\ghub-per\titan-alloy\irrgo\images",
    6,    # pixel margin around tight bounding box
    5,    # columns
    3     # rows
)
