const fs = require('fs');
const process = require('process');

const COLORS_PER_PALETTE = 16;
const PIXELS_PER_ROW = 16;
const ROWS_PER_TILE = 16;

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

            // console.log(r, g, b, packed.toString(16));
            return packed;
        }));
    }

    // filter out all-zero palettes at the end. inner zero palettes are ok
    return palettes.slice(0, palettes.findLastIndex(palette => !palette.every(color => color === 0)) + 1);
}

function makeSubtiles(tile) {
    let pixels = [];
    for (let k = 0; k < tile.data.length; k += PIXELS_PER_ROW) {
        pixels.push(tile.data.slice(k, k + PIXELS_PER_ROW));
    }

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

if (process.argv.length < 3) {
    console.log('no input specified');
    process.exit(1);
}

const inData = readJsonFile(process.argv[2]);
const outName = inData.name.replace(/\s/g, '-');

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

const tileDataLen = 0x400 + 32 * 4 * inData.tiles.length;
const tileDataBuf = new ArrayBuffer(tileDataLen);
const tileDataBytes = new Uint8Array(tileDataBuf);

let outIndex = 0;
inData.tiles.forEach(tile => {
    let subtiles = makeSubtiles(tile);
    let topLeft = convert(subtiles.topLeft);
    let topRight = convert(subtiles.topRight);
    let bottomLeft = convert(subtiles.bottomLeft);
    let bottomRight = convert(subtiles.bottomRight);
    for (let k = 0; k < topLeft.length; k++) {
        tileDataBytes[outIndex] = topLeft[k];
        tileDataBytes[0x200 + outIndex] = bottomLeft[k];

        outIndex++;
    }

    for (let k = 0; k < topLeft.length; k++) {
        tileDataBytes[outIndex] = topRight[k];
        tileDataBytes[0x200 + outIndex] = bottomRight[k];

        outIndex++;
    }
});
fs.writeFileSync(`${outName}.4bp`, Buffer.from(tileDataBuf));

console.log(paletteDataShorts);
