function exportArrayBuffer(buf, name) {
    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([buf], { type: 'application/octet-stream' }));
    a.setAttribute('download', name);
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
}

function exportPalettes() {
    const palettes = convertPalettes();
    const paletteDataLen = 2 * COLORS_PER_PALETTE * palettes.length;
    const paletteDataBuf = new ArrayBuffer(paletteDataLen);
    const paletteDataShorts = new Uint16Array(paletteDataBuf);
    for (let p = 0; p < palettes.length; p++) {
        for (let k = 0; k < COLORS_PER_PALETTE; k++) {
            paletteDataShorts[p * COLORS_PER_PALETTE + k] = palettes[p][k];
        }
    }
    exportArrayBuffer(paletteDataBuf, `${getProjectName()}.pal`);
}

function exportTiles() {
    // Add blank tile to tiles array for export
    const blankTile = {
        data: new Array(tileSize * tileSize).fill(0),
        palette: 0,
        canvas: null
    };
    const tilesWithBlank = [...tiles, blankTile];
    
    let tileDataLen = 32 * tilesWithBlank.length;
    if (tileSize === 16) {
        // round up size to nearest multiple of 1024 to ensure space for bottom row
        tileDataLen = Math.ceil((32 * 4 * tilesWithBlank.length) / 0x400) * 0x400;
    }
    const tileDataBuf = new ArrayBuffer(tileDataLen);
    const tileDataBytes = new Uint8Array(tileDataBuf);

    if (tileSize === 16) {
        write16(tilesWithBlank, tileDataBytes);
    } else {
        write8(tilesWithBlank, tileDataBytes);
    }

    exportArrayBuffer(tileDataBuf, `${getProjectName()}.4bp`);
}

function exportBackground() {
    if (tiles.length === 0) {
        alert('No tiles');
        return;
    }

    const tilemapDataLen = 2048;
    const tilemapDataBuf = new ArrayBuffer(tilemapDataLen);
    const tilemapDataShorts = new Uint16Array(tilemapDataBuf);
    const paletteOffset = parseInt(document.getElementById('palette-index-offset').value);
    
    // blank tile will be at index tiles.length after all real tiles
    const blankTileIndex = tiles.length;

    let outIndex = 0;
    for (let y = 0; y < BG_HEIGHT_TILES; y++) {
        for (let x = 0; x < BG_WIDTH_TILES; x++) {
            let tileNum = background[y][x];
            if (tileNum === -1) {
                tileNum = blankTileIndex;
            }

            let paletteNum = 0; // Default palette for blank tile
            if (tileNum !== -1) {
                paletteNum = tiles[tileNum].palette + paletteOffset;
            }

            let tileNum8x8 = tileNum;
            if (tileSize === 16) {
                tileNum8x8 = 2 * (tileNum % 8) + 32 * Math.floor(tileNum / 8);
            }
    
            tilemapDataShorts[outIndex++] = paletteNum << 10 | tileNum8x8;
        }
    }

    exportArrayBuffer(tilemapDataBuf, `${getProjectName()}.map`);
}

function exportPng() {
    const a = document.createElement('a');
    a.setAttribute('download', `${getProjectName()}.png`);
    bgCanvas.toBlob(blob => {
        const url = URL.createObjectURL(blob);
        a.setAttribute('href', url);
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
    });
}

function convertPalettes() {
    const colors = paletteEntries.map(entry => entry.value);
    const palettes = [];
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