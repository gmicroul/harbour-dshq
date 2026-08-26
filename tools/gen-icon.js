#!/usr/bin/env node
// Launcher icon: chibi "big fat fish" whale girl (DeepSeek mascot style).
// 86x86 RGBA PNG, hand-rolled encoder — no external image tools needed.
const zlib = require("zlib");
const fs = require("fs");

const W = 86, H = 86;
const M = 4, R = 16;          // rounded-square mask
const DARK = [27, 42, 94];

const clamp01 = v => Math.max(0, Math.min(1, v));
function inEllipse(x, y, cx, cy, rx, ry, rotDeg = 0) {
    const dx = x - cx, dy = y - cy;
    let u, v;
    if (rotDeg) {
        const a = rotDeg * Math.PI / 180;
        u = dx * Math.cos(a) + dy * Math.sin(a);
        v = -dx * Math.sin(a) + dy * Math.cos(a);
    } else { u = dx; v = dy; }
    return (u * u) / (rx * rx) + (v * v) / (ry * ry) <= 1;
}
const lerp = (a, b, t) => Math.round(a + (b - a) * t);

function pixel(x, y) {
    // ---- rounded-square background mask (DeepSeek blue->purple gradient)
    if (x < M || x > W - M || y < M || y > H - M) return [0, 0, 0, 0];
    const cx = Math.max(M + R, Math.min(x, W - M - R));
    const cy = Math.max(M + R, Math.min(y, H - M - R));
    const ddx = x - cx, ddy = y - cy;
    if (ddx * ddx + ddy * ddy > R * R) return [0, 0, 0, 0];
    const tg = clamp01((x + y) / (W + H));
    const bg = [lerp(77, 138, tg), lerp(107, 82, tg), lerp(254, 255, tg)];

    // ---- water spout (three droplet arcs above the head)
    for (let t = 1; t <= 4; t++) {
        for (const s of [-1, 1]) {
            const sx = 43 + s * t * 2.4, sy = 19 - t * 3.2;
            if ((x - sx) ** 2 + (y - sy) ** 2 <= 2.9) return [208, 234, 255, 255];
        }
    }

    // ---- tail flukes (right side)
    if (inEllipse(x, y, 71, 40, 8, 5.5) || inEllipse(x, y, 73, 54, 8, 6) ||
        inEllipse(x, y, 64, 47, 5, 4))
        return [100, 165, 245, 255];

    // ---- chubby body
    const inBody = inEllipse(x, y, 40, 47, 29, 23);
    if (inBody) {
        const tb = clamp01((y - 24) / 46);
        const gradient = () => [lerp(158, 108, tb), lerp(216, 176, tb), 255, 255];

        // face first: nothing below may cover it
        for (const ex of [33, 53]) {
            if (inEllipse(x, y, ex, 43, 3.9, 4.7)) {
                if ((x - (ex - 1.3)) ** 2 + (y - 41.4) ** 2 <= 2.4) return [255, 255, 255, 255];
                return [...DARK, 255];
            }
            // eyelashes: two short strokes at the outer corner
            const s = ex === 33 ? -1 : 1;
            if (Math.abs(y - 37.6) < 0.9 && Math.abs(x - (ex + s * 4.6)) < 1.6) return [...DARK, 255];
            if (Math.abs(y - 39.2) < 0.9 && Math.abs(x - (ex + s * 6.6)) < 1.4) return [...DARK, 255];
        }
        // blush (overlaps the belly's upper edge -> must win)
        for (const bx of [25, 61])
            if (inEllipse(x, y, bx, 50, 3.4, 2.6)) return [255, 158, 184, 235];
        // smile (small parabola)
        if (x >= 39 && x <= 47 && Math.abs(y - (46.6 + 0.1 * (x - 43) ** 2)) < 1)
            return [...DARK, 255];

        // left flipper (in front of the belly's left edge)
        if (inEllipse(x, y, 22, 57, 6.5, 4)) return [88, 150, 232, 255];
        // belly
        if (inEllipse(x, y, 38, 58, 20, 11)) return [241, 250, 255, 255];

        return gradient();
    }

    // ---- bow (headband girl touch), drawn after body so it sits on the head
    if (inEllipse(x, y, 50.5, 23, 6.2, 4.6, -18)) return [228, 86, 138, 255];
    if (inEllipse(x, y, 61.5, 23, 6.2, 4.6, 18)) return [228, 86, 138, 255];
    if (inEllipse(x, y, 50.5, 23, 4.6, 3.3, -18)) return [255, 132, 172, 255];
    if (inEllipse(x, y, 61.5, 23, 4.6, 3.3, 18)) return [255, 132, 172, 255];
    if (inEllipse(x, y, 56, 23.5, 3.6, 3.6)) {
        if ((x - 55) ** 2 + (y - 22.3) ** 2 <= 1.2) return [255, 205, 225, 255];
        return [255, 104, 152, 255];
    }

    return [...bg, 255];
}

// ---- encode PNG
const raw = Buffer.alloc(H * (1 + W * 4));
let o = 0;
for (let y = 0; y < H; y++) {
    raw[o++] = 0;
    for (let x = 0; x < W; x++) {
        const [r, g, b, a] = pixel(x, y);
        raw[o++] = r; raw[o++] = g; raw[o++] = b; raw[o++] = a;
    }
}
const crcTable = [];
for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    crcTable[n] = c >>> 0;
}
const crc32 = buf => {
    let c = 0xffffffff;
    for (const byte of buf) c = crcTable[(c ^ byte) & 0xff] ^ (c >>> 8);
    return (c ^ 0xffffffff) >>> 0;
};
const chunk = (type, data) => {
    const len = Buffer.alloc(4);
    len.writeUInt32BE(data.length);
    const body = Buffer.concat([Buffer.from(type, "ascii"), data]);
    const crc = Buffer.alloc(4);
    crc.writeUInt32BE(crc32(body));
    return Buffer.concat([len, body, crc]);
};
const ihdr = Buffer.alloc(13);
ihdr.writeUInt32BE(W, 0);
ihdr.writeUInt32BE(H, 4);
ihdr[8] = 8;
ihdr[9] = 6;
const png = Buffer.concat([
    Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
    chunk("IHDR", ihdr),
    chunk("IDAT", zlib.deflateSync(raw, { level: 9 })),
    chunk("IEND", Buffer.alloc(0))
]);
fs.writeFileSync(process.argv[2] || "harbour-dshq.png", png);
console.log("wrote", process.argv[2] || "harbour-dshq.png", png.length, "bytes");

// optional: nearest-neighbour preview at N x (argv[3], e.g. 4 -> 344px)
if (process.argv[3] && Number(process.argv[3]) > 1) {
    const S = Math.floor(Number(process.argv[3]));
    const PW = W * S, PH = H * S;
    const praw = Buffer.alloc(PH * (1 + PW * 4));
    let po = 0;
    for (let y = 0; y < PH; y++) {
        praw[po++] = 0;
        const sy = Math.floor(y / S);
        for (let x = 0; x < PW; x++) {
            const sx = Math.floor(x / S);
            const src = 1 + (sy * (1 + W * 4)) + sx * 4 + 1;
            praw[po++] = raw[src - 1];
            praw[po++] = raw[src];
            praw[po++] = raw[src + 1];
            praw[po++] = raw[src + 2];
        }
    }
    const pihdr = Buffer.from(ihdr);
    pihdr.writeUInt32BE(PW, 0);
    pihdr.writeUInt32BE(PH, 4);
    const ppng = Buffer.concat([
        Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
        chunk("IHDR", pihdr),
        chunk("IDAT", zlib.deflateSync(praw, { level: 9 })),
        chunk("IEND", Buffer.alloc(0))
    ]);
    fs.writeFileSync(process.argv[4] || "harbour-dshq-preview.png", ppng);
    console.log("wrote", process.argv[4] || "harbour-dshq-preview.png", ppng.length, "bytes");
}
