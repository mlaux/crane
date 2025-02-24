const fs = require('fs');
const process = require('process');

const COLORS_PER_PALETTE = 16;

const readJsonFile = filePath => {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
};

function convertPalettes(colors) {
    let palettes = [];
    for (let k = 0; k < colors.length; k += COLORS_PER_PALETTE) {
        palettes.push(colors.slice(k, k + COLORS_PER_PALETTE).map(col => {
            const val = parseInt(col.substring(1), 16);
            
            let r = val >> 19 & 0x1f;
            let g = val >> 11 & 0x1f;
            let b = val >> 3 & 0x1f;

            let packed = (b << 10) | (g << 5) | r;

            return packed;
        }));
    }

    // filter out all-zero palettes at the end. inner zero palettes are ok
    return palettes.slice(0, palettes.findLastIndex(palette => !palette.every(color => color === 0)) + 1);
}

function make2d(tile, size) {
    const pixels = [];
    for (let k = 0; k < tile.data.length; k += size) {
        pixels.push(tile.data.slice(k, k + size));
    }
    return pixels;
}

function makeSubtiles(tile) {
    const pixels = make2d(tile, 16);
    let top = pixels.slice(0, 8);
    let bottom = pixels.slice(8, 16);

    let topLeft = top.map(row => row.slice(0, 8));
    let topRight = top.map(row => row.slice(8, 16));
    let bottomLeft = bottom.map(row => row.slice(0, 8));
    let bottomRight = bottom.map(row => row.slice(8, 16));
    return { topLeft, topRight, bottomLeft, bottomRight };
}

function convert(tile) {
    const bit0 = tile.map(row => row.map(pix => pix & 1).join('')).map(str => parseInt(str, 2));
    const bit1 = tile.map(row => row.map(pix => pix >> 1 & 1).join('')).map(str => parseInt(str, 2));
    const bit2 = tile.map(row => row.map(pix => pix >> 2 & 1).join('')).map(str => parseInt(str, 2));
    const bit3 = tile.map(row => row.map(pix => pix >> 3 & 1).join('')).map(str => parseInt(str, 2));

    const out = [];
    for (let k = 0; k < bit0.length; k++) {
        out.push(bit0[k]);
        out.push(bit1[k]);
    }
    for (let k = 0; k < bit0.length; k++) {
        out.push(bit2[k]);
        out.push(bit3[k]);
    }

    return out;
}

function write8(tiles, out) {
    let outIndex = 0;
    tiles.forEach(tile => {
        let data = convert(make2d(tile, 8));
        for (let k = 0; k < data.length; k++) {
            out[outIndex++] = data[k];
        }
    });
}

function write16(tiles, out) {
    let outIndex = 0;
    tiles.forEach(tile => {
        let subtiles = makeSubtiles(tile);
        let topLeft = convert(subtiles.topLeft);
        let topRight = convert(subtiles.topRight);
        let bottomLeft = convert(subtiles.bottomLeft);
        let bottomRight = convert(subtiles.bottomRight);
        for (let k = 0; k < topLeft.length; k++) {
            out[outIndex] = topLeft[k];
            out[outIndex + 32] = topRight[k];
            out[outIndex + 0x200] = bottomLeft[k];
            out[outIndex + 0x200 + 32] = bottomRight[k];

            outIndex++;
        }

        outIndex += 32;
        if (outIndex % 0x200 == 0) {
            outIndex += 0x200;
        }
    });
}

if (process.argv.length < 3) {
    console.log('no input specified');
    process.exit(1);
}

const inData = readJsonFile(process.argv[2]);
const outName = inData.name.replace(/\s/g, '-');
const tileSize = inData.tileSize || 16;
const paletteOffset = parseInt(process.argv[3]) || 0;

const palettes = convertPalettes(inData.colors);
const paletteDataLen = 2 * COLORS_PER_PALETTE * palettes.length;
const paletteDataBuf = new ArrayBuffer(paletteDataLen);
const paletteDataShorts = new Uint16Array(paletteDataBuf);
for (let p = 0; p < palettes.length; p++) {
    for (let k = 0; k < COLORS_PER_PALETTE; k++) {
        paletteDataShorts[p * COLORS_PER_PALETTE + k] = palettes[p][k];
    }
}

fs.writeFileSync(`${outName}.pal`, Buffer.from(paletteDataBuf));
console.log(`Wrote ${palettes.length} palettes to ${outName}.pal`);

let tileDataLen = 32 * inData.tiles.length;
if (tileSize === 16) {
    // round up size to nearest multiple of 1024 to ensure space for bottom row
    tileDataLen = Math.ceil((32 * 4 * inData.tiles.length) / 0x400) * 0x400;
}
const tileDataBuf = new ArrayBuffer(tileDataLen);
const tileDataBytes = new Uint8Array(tileDataBuf);

if (tileSize === 16) {
    write16(inData.tiles, tileDataBytes);
} else {
    write8(inData.tiles, tileDataBytes);
}

fs.writeFileSync(`${outName}.4bp`, Buffer.from(tileDataBuf));
console.log(`Wrote ${inData.tiles.length} tiles to ${outName}.4bp`);

const tilemapDataLen = 2048;
const tilemapDataBuf = new ArrayBuffer(tilemapDataLen);
const tilemapDataShorts = new Uint16Array(tilemapDataBuf);

// some files can have 16x16 backgrounds
if (inData.background.length < 32) {
    const newRows = new Array(32 - inData.background.length).fill()
            .map(_ => []);
    inData.background = inData.background.concat(newRows);
}

outIndex = 0;
for (let y = 0; y < 32; y++) {
    if (inData.background[y].length < 32) {
        inData.background[y] = inData.background[y].concat(
                new Array(32 - inData.background[y].length).fill(0));
    }
    for (let x = 0; x < 32; x++) {
        let tileNum = inData.background[y][x];
        if (tileNum === -1) {
            tileNum = 0;
        }
        let useTileNum = tileNum;
        if (tileSize === 16) {
            useTileNum = 2 * (tileNum % 8) + 32 * Math.floor(tileNum / 8);
        }
        const paletteNum = inData.tiles[tileNum].palette + paletteOffset;

        tilemapDataShorts[outIndex++] = paletteNum << 10 | useTileNum;
    }
}

fs.writeFileSync(`${outName}.map`, Buffer.from(tilemapDataBuf));
console.log(`Wrote background to ${outName}.map`);
