// DOM elements of the pixels in the tile editor
const pixels = [];
// <input type=color>s for the main palette section
const paletteEntries = [];
// <input type=color>s for the tile editor
const editorPaletteEntries = [];

// list of tiles in the tile library
const tiles = [];
let editedTile = null;
let selectedTileIndex = -1;

// 2d array of indices into the tiles array
const background = [];

// for tile editor. TODO allow zooming the BG as well
const ZOOMS = [8, 16, 32];
let zoomIndex = 1;

const COLORS_PER_PALETTE = 16;

const BG_WIDTH_TILES = 32;
const BG_HEIGHT_TILES = 32;

// TODO configurable
const BG_SCALE_FACTOR = 2;

const bgCanvas = document.getElementById('background-canvas');
const overlay = document.getElementById('background-overlay');

const editorOverlay = document.getElementById('tile-editor-overlay');
const editor = document.getElementById('tile-editor');
const editorPaletteSelector = document.getElementById('editor-palette-selector');

const DEFAULT_TILE_SIZE = 16;
let tileSize;

// empty means no tiles, all blank background, all black palettes
function isDocumentEmpty() {
    return !tiles.length 
            && !background.some(row => row.some(el => el != -1))
            && !paletteEntries.some(el => el.value != '#000000');
}

function resizePixels() {
    pixels.forEach(row => {
        row.forEach(it => {
            it.style.width = `${ZOOMS[zoomIndex]}px`;
            it.style.height = `${ZOOMS[zoomIndex]}px`;
        })
    });
}

function zoomIn() {
    if (zoomIndex < ZOOMS.length - 1) {
        zoomIndex++;
    }
    resizePixels();
}

function zoomOut() {
    if (zoomIndex > 0) {
        zoomIndex--;
    }
    resizePixels();
}

function parseData(data) {
    // TODO fix this, should have the DOM value be the only tile size.
    // should not have 2 variables to keep in sync (radio button checked and
    // tileSize int)
    let size = data['tileSize'];
    if (!size) {
        size = DEFAULT_TILE_SIZE;
    }
    initialize(size);

    if (size === 16) {
        document.getElementById('tile-size-16').checked = true;
    } else if (size === 8) {
        document.getElementById('tile-size-8').checked = true;
    } else {
        alert('Invalid tile size - must be 8 or 16');
        return;
    }

    document.getElementById('project-name').value = data['name'];

    for (let k = 0; k < data['colors'].length; k++) {
        paletteEntries[k].value = data['colors'][k];
    }

    for (let k = 0; k < data['tiles'].length; k++) {
        loadTile(data['tiles'][k]);
    }

    if (!data.background) {
        // skip for earlier files that only have colors/tiles
        return;
    }

    for (let y = 0; y < data.background.length; y++) {
        for (let x = 0; x < data.background[y].length; x++) {
            background[y][x] = data.background[y][x];
        }
    }
    redrawBackground();
}

function loadPalettes() {
    const input = document.createElement('input');
    input.type = 'file';
    document.body.appendChild(input);

    input.addEventListener('change', evt => { 
        const file = evt.target.files[0]; 
        const reader = new FileReader();
        reader.readAsText(file, 'UTF-8');
        reader.onload = readerEvent => {
            parseData(JSON.parse(readerEvent.target.result));
        };
        reader.onloadend = () => {
            document.body.removeChild(input);
        };
    });

    input.click();
}

function getProjectName() {
    return document.getElementById('project-name').value || 'Untitled';
}

function savePalettes() {
    const colors = [];
    const outTiles = [];
    const name = getProjectName();
    for (let k = 0; k < paletteEntries.length; k++) {
        colors[k] = paletteEntries[k].value;
    }
    for (let k = 0; k < tiles.length; k++) {
        outTiles[k] = {
            palette: tiles[k].palette,
            data: tiles[k].data,
        };
    }
    const out = {
        name,
        colors,
        tiles: outTiles,
        background,
        tileSize,
    };

    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([JSON.stringify(out)], {
        type: 'text/plain'
    }));
    a.setAttribute('download', `${name}.json`);
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
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
    let tileDataLen = 32 * tiles.length;
    if (tileSize === 16) {
        // round up size to nearest multiple of 1024 to ensure space for bottom row
        tileDataLen = Math.ceil((32 * 4 * tiles.length) / 0x400) * 0x400;
    }
    const tileDataBuf = new ArrayBuffer(tileDataLen);
    const tileDataBytes = new Uint8Array(tileDataBuf);

    if (tileSize === 16) {
        write16(tiles, tileDataBytes);
    } else {
        write8(tiles, tileDataBytes);
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

    let outIndex = 0;
    for (let y = 0; y < BG_HEIGHT_TILES; y++) {
        for (let x = 0; x < BG_WIDTH_TILES; x++) {
            let tileNum = background[y][x];
            if (tileNum === -1) {
                tileNum = 0; // TODO
            }

            let useTileNum = tileNum;
            if (tileSize === 16) {
                useTileNum = 2 * (tileNum % 8) + 32 * Math.floor(tileNum / 8);
            }
            const paletteNum = tiles[tileNum].palette + paletteOffset;
    
            tilemapDataShorts[outIndex++] = paletteNum << 10 | useTileNum;
        }
    }

    exportArrayBuffer(tilemapDataBuf, `${getProjectName()}.map`);
}

function selectPaletteEntry(entry) {
    for (let k = 0; k < editorPaletteEntries.length; k++) {
        editorPaletteEntries[k].classList.remove('palette-entry-selected');
    }
    entry.classList.add('palette-entry-selected');
}

function selectTile(tile) {
    selectedTileIndex = tiles.indexOf(tile);
    for (let k = 0; k < tiles.length; k++) {
        tiles[k].canvas.classList.remove('tile-selected');
    }
    tile.canvas.classList.add('tile-selected');

    updateOverlay(tile);
}

function highlightTile(tile) {
    for (let k = 0; k < tiles.length; k++) {
        tiles[k].canvas.classList.remove('tile-highlighted');
    }
    if (tile) {
        tile.canvas.classList.add('tile-highlighted');
    }
}

function deleteTile() {
    if (selectedTileIndex === -1) {
        return;
    }

    const tile = tiles[selectedTileIndex];
    // remove from the main list
    tiles.splice(selectedTileIndex, 1);

    // remove from the tile list in the document
    document.getElementById('tile-entries').removeChild(tile.canvas);

    // remove deleted tile from the background and shift tiles above it down one
    for (let y = 0; y < BG_HEIGHT_TILES; y++) {
        for (let x = 0; x < BG_WIDTH_TILES; x++) {
            if (background[y][x] > selectedTileIndex) {
                background[y][x]--;
            } else if (background[y][x] === selectedTileIndex) {
                background[y][x] = -1;
            }
        }
    }
    overlay.getContext('2d').clearRect(0, 0, overlay.width, overlay.height);

    redrawBackground();
    selectedTileIndex = -1;
}

function makePalette(text, forTileEditor) {
    const row = document.createElement('div');
    row.classList.add('palette-row');
    row.style.display = 'inline-block';
    if (text !== null) {
        row.innerText = text + ' ';
    }
    for (let x = 0; x < 16; x++) {
        const entry = document.createElement('input');
        entry.type = 'color';
        entry.style.verticalAlign = 'middle';
        entry.classList.add('palette-entry');
        entry.className = 'palette-entry';
        if (forTileEditor) {
            entry.classList.add('palette-entry-editor');
            entry.onclick = function(evt) {
                if (!evt.shiftKey) {
                    evt.preventDefault();
                    selectPaletteEntry(entry);
                }
            };
            entry.onchange = onTileEditorPaletteEntryChanged;
            editorPaletteEntries.push(entry);
        } else {
            entry.classList.add('palette-entry-global');
            entry.onchange = onGlobalPaletteEntryChanged;
            paletteEntries.push(entry);
        }
        if (x != 0) {
            row.appendChild(entry);
        }
    }
    return row;
}

function onTileEditorPaletteEntryChanged() {
    copyEditorPaletteToGlobal(parseInt(editorPaletteSelector.value));
    redrawPixels();
    redrawTiles();
}

function onGlobalPaletteEntryChanged() {
    redrawTiles();
    redrawBackground();
}

function openTileEditor(tile) {
    if (!tile) {
        tile = createTile();
    }
    editedTile = tile;

    for (let y = 0; y < tileSize; y++) {
        for (let x = 0; x < tileSize; x++) {
            pixels[y][x].setAttribute('data-palette-index', tile.data[y * tileSize + x].toString());
        }
    }

    editorOverlay.style.display = 'flex';
    editorPaletteSelector.value = tile.palette.toString();
    editorPaletteSelector.onchange();
}

function createTile() {
    // this is 1d and the background is 2d... why
    const indices = new Array(tileSize * tileSize).fill(0);
    const canvas = document.createElement('canvas');
    const tile = {
        data: indices,
        palette: parseInt(editorPaletteSelector.value),
        canvas: canvas,
    };

    canvas.className = 'tile';
    canvas.width = tileSize;
    canvas.height = tileSize;
    canvas.style.width = `${4 * tileSize}px`;
    canvas.style.height = `${4 * tileSize}px`;
    canvas.style.backgroundColor = 'white';
    canvas.onclick = function(evt) {
        if (evt.shiftKey) {
            openTileEditor(tile);
        } else {
            selectTile(tile);
        }
    };

    document.getElementById('tile-entries').appendChild(canvas);
    tiles.push(tile);
    return tile;
}

function placeTile(x, y, remove) {
    background[y][x] = remove ? -1 : selectedTileIndex;
    redrawBackground();
}

function placePixel(pix, remove) {
    if (remove) {
        pix.setAttribute('data-palette-index', '0');
    } else {
        const selectedEntries = document.getElementsByClassName('palette-entry-selected');
        if (selectedEntries.length) {
            const selectedEntry = selectedEntries[0];
            const index = editorPaletteEntries.indexOf(selectedEntry);
            pix.setAttribute('data-palette-index', index.toString());
        }
    }
}

function closeTileEditor() {
    editorOverlay.style.display = 'none';

    // copy from pixel editor to tile data
    for (let y = 0; y < tileSize; y++) {
        for (let x = 0; x < tileSize; x++) {
            const colorIndex = parseInt(pixels[y][x].getAttribute('data-palette-index'));
            editedTile.data[y * tileSize + x] = colorIndex;
        }
    }

    // editing a tile affects both the tile and the background
    redrawTile(editedTile);
    redrawBackground();
    // don't update overlay if editing a tile that's not the selected one
    if (selectedTileIndex !== -1 && editedTile === tiles[selectedTileIndex]) {
        updateOverlay(editedTile);
    }
    editedTile = null;
}

function loadTile(data) {
    const tile = createTile();
    tile.data = data.data;
    tile.palette = data.palette;
    redrawTile(tile);
}

function redrawTile(tile) {
    const ctx = tile.canvas.getContext('2d');
    const image = ctx.createImageData(tileSize, tileSize);
    const basePaletteIndex = tile.palette * 16;

    for (let y = 0; y < tileSize; y++) {
        for (let x = 0; x < tileSize; x++) {
            const index = 4 * (y * tileSize + x);
            const colorIndex = parseInt(tile.data[y * tileSize + x]);
            if (colorIndex != 0) {
                const color = parseInt(paletteEntries[basePaletteIndex + colorIndex].value.substring(1), 16);

                image.data[index] = color >> 16 & 0xff;
                image.data[index + 1] = color >> 8 & 0xff;
                image.data[index + 2] = color & 0xff;
                image.data[index + 3] = 0xff;
            }
        }
    }
    ctx.putImageData(image, 0, 0);
}

function redrawTiles() {
    tiles.forEach(redrawTile);
}

function redrawBackground() {
    const bgCanvas = document.getElementById('background-canvas');
    const bgContext = bgCanvas.getContext('2d');

    for (let y = 0; y < BG_HEIGHT_TILES; y++) {
        for (let x = 0; x < BG_WIDTH_TILES; x++) {
            bgContext.clearRect(x * tileSize, y * tileSize, tileSize, tileSize)
            if (background[y][x] !== -1) {
                const tile = tiles[background[y][x]];
                bgContext.drawImage(tile.canvas, x * tileSize, y * tileSize);
            }
        }
    }
}

function updateOverlay(tile) {
    const ctx = overlay.getContext('2d');

    ctx.clearRect(0, 0, overlay.width, overlay.height);
    ctx.drawImage(tile.canvas, 0, 0);
}

function canvasToTile(c) {
    for (let k = 0; k < tiles.length; k++) {
        if (tiles[k].canvas === c) {
            return tiles[k];
        }
    }
    return null;
}

function copyPaletteToEditor(index) {
    const base = index * 16;
    for (let k = 0; k < 16; k++) {
        editorPaletteEntries[k].value = paletteEntries[base + k].value;
    }
}

function copyEditorPaletteToGlobal(index) {
    const base = index * 16;
    for (let k = 0; k < 16; k++) {
        paletteEntries[base + k].value = editorPaletteEntries[k].value;
    }
}

// copies data from the custom attribute to the styles
function redrawPixels() {
    for (let y = 0; y < tileSize; y++) {
        for (let x = 0; x < tileSize; x++) {
            const colorIndex = pixels[y][x].getAttribute('data-palette-index');
            if (colorIndex == 0) {
                pixels[y][x].style.backgroundImage = 'url("transparent.png")';
                pixels[y][x].setAttribute('data-palette-index', '0');
            } else {
                const color = editorPaletteEntries[colorIndex].value;
                pixels[y][x].style.backgroundImage = '';
                pixels[y][x].style.backgroundColor = color;
            }
        }
    }
}

function importPalette() {
    const urlField = document.getElementById('palette-import-url');
    let url = urlField.value;
    if (!url) {
        return;
    }
    url += '.json';

    let where = parseInt(document.getElementById('palette-import-index').value);
    if (!where || where < 0 || where > 7) {
        where = 0;
    }

    (async () => {
        const response = await fetch(url);
        if (!response.ok) {
            return;
        }

        const obj = await response.json();

        const baseIndex = where * 16 + 1; // +1 to skip transparent
        for (let k = 0; k < obj.colors.length && k < 15; k++) {
            paletteEntries[baseIndex + k].value = `#${obj.colors[k]}`;
        }

        urlField.value = '';

        redrawTiles();
        redrawBackground();
        if (selectedTileIndex !== -1) {
            updateOverlay(tiles[selectedTileIndex]);
        }
    })();
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

function initializePalettes() {
    const paletteContainer = document.getElementById('palettes');
    const editorPaletteContainer = document.getElementById('editor-palette-area');

    if (paletteContainer.childNodes.length) {
        // palettes are unique in that the elements are kept - 
        // tiles, bg, and editor pixels get recreated
        paletteEntries.forEach(el => el.value = '#000000');
        editorPaletteEntries.forEach(el => el.value = '#000000');
    } else {
        for (let pn = 0; pn < 8; pn++) {
            const palette = makePalette(pn);
            paletteContainer.appendChild(palette);
        }
    
        const editorPalette = makePalette(null, true);
        editorPaletteContainer.appendChild(editorPalette);
    }
}

function initializeTiles() {
    tiles.length = 0;
    document.getElementById('tile-entries').innerHTML = '';
}

function initializePixels() {
    const pixelsContainer = document.getElementById('pixels-container');
    pixelsContainer.innerHTML = '';
    pixels.length = 0;
    for (let y = 0; y < tileSize; y++) {
        pixels[y] = [];
        const row = document.createElement('div');
        row.className = 'pixel-row';
        for (let x = 0; x < tileSize; x++) {
            const pix = document.createElement('div');
            pix.className = 'pixel';
            pix.style.backgroundImage = 'url("transparent.png")';
            pix.style.backgroundRepeat = 'repeat';
            pix.setAttribute('data-palette-index', '0');
    
            pix.onpointerdown = function(evt) {
                placePixel(pix, evt.shiftKey);
                redrawPixels();
            };
            pix.onpointermove = function(evt) {
                // need to look up the pixel every time. for touch/pen events,
                // the initially touched pixel serves as the reference for the move
                // events too
                // THIS DEPENDS ON box-sizing: border-box on the main container
                let bounds = pixelsContainer.getBoundingClientRect();
                let useX = Math.floor((evt.clientX - bounds.left) / 16);
                let useY = Math.floor((evt.clientY - bounds.top) / 16);
                const curPix = pixels[useY][useX];

                if (evt.buttons || evt.pointerType === 'pen' || evt.pointerType === 'touch') {
                    placePixel(curPix, evt.shiftKey);
                    redrawPixels();
                }
            }
            pix.onscroll = function(evt) {
                evt.preventDefault();
            }
            pixels[y].push(pix);
            row.appendChild(pix);
        }
    
        pixelsContainer.appendChild(row);
    }
}

function initializeBackground() {
    bgCanvas.width = tileSize * BG_WIDTH_TILES;
    bgCanvas.height = tileSize * BG_HEIGHT_TILES;
    bgCanvas.style.width = `${tileSize * BG_WIDTH_TILES * BG_SCALE_FACTOR}px`;
    bgCanvas.style.height = `${tileSize * BG_HEIGHT_TILES * BG_SCALE_FACTOR}px`;

    overlay.width = tileSize;
    overlay.height = tileSize;
    overlay.style.width = `${tileSize * BG_SCALE_FACTOR}px`;
    overlay.style.height = `${tileSize * BG_SCALE_FACTOR}px`;

    background.length = 0;
    for (let y = 0; y < BG_HEIGHT_TILES; y++) {
        background[y] = new Array(BG_WIDTH_TILES).fill(-1);
    }
}

function addEventHandlers() {
    editorOverlay.onclick = function(evt) {
        const els = document.elementsFromPoint(evt.x, evt.y);
        // don't close if clicked inside the editor
        if (els.indexOf(editor) === -1) {
            closeTileEditor();
        }
    };
    
    editorPaletteSelector.onchange = function() {
        const num = parseInt(editorPaletteSelector.value);
        editedTile.palette = num;
        copyPaletteToEditor(num);
        redrawPixels();
    };
    
    document.getElementById('zoom-in').onclick = function(e) {
        zoomIn();
        e.stopPropagation();
    }
    
    document.getElementById('zoom-out').onclick = function(e) {
        zoomOut();
        e.stopPropagation();
    }

    bgCanvas.onmousemove = function(e) {
        const scale = BG_SCALE_FACTOR * tileSize;
        const x = Math.floor(e.offsetX / scale);
        const y = Math.floor(e.offsetY / scale);
        overlay.style.left = `${x * scale}px`;
        overlay.style.top = `${y * scale}px`;

        if (background[y][x] !== -1) {
            highlightTile(tiles[background[y][x]]);
        } else {
            highlightTile(null);
        }

        if (e.buttons) {
            placeTile(x, y, e.shiftKey);
        }
    }

    bgCanvas.onclick = function(e) {
        const x = Math.floor(e.offsetX / (BG_SCALE_FACTOR * tileSize));
        const y = Math.floor(e.offsetY / (BG_SCALE_FACTOR * tileSize));

        placeTile(x, y, e.shiftKey);
    }

    document.getElementById('tile-size-8').onclick = () => {
        initialize(8);
    };

    document.getElementById('tile-size-16').onclick = () => {
        initialize(16);
    };

    document.body.onpointerdown = evt => {
        //console.log(evt.pointerType);
        // if (evt.pointerType === 'pen') {
        //     console.log('is pen');
        // }
    };

    document.body.onscroll = e => e.preventDefault();
    
    window.onbeforeunload = function() {
        return true;
    };
}

function initialize(size) {
    if (!isDocumentEmpty() && !confirm('Any unsaved changes will be lost, continue?')) {
        return;
    }
    tileSize = size;
    document.getElementById('project-name').value = '';
    initializePalettes();
    initializeTiles();
    initializePixels();
    initializeBackground();
}

initialize(DEFAULT_TILE_SIZE);
addEventHandlers();