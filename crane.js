const pixels = [];
const paletteEntries = [];
const editorPaletteEntries = [];

const tiles = [];
let editedTile = null;
let selectedTileIndex = -1;

const background = [];

const ZOOMS = [8, 16, 32];
let zoomIndex = 1;

const BG_WIDTH_TILES = 16;
const BG_HEIGHT_TILES = 14;
const TILE_SIZE = 16;

const editorOverlay = document.getElementById('tile-editor-overlay');
const editor = document.getElementById('tile-editor');
const editorPaletteSelector = document.getElementById('editor-palette-selector');

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
    document.getElementById('project-name').value = data['name'];

    for (let k = 0; k < data['colors'].length; k++) {
        paletteEntries[k].value = data['colors'][k];
    }

    tiles.length = 0;
    document.getElementById('tile-entries').innerHTML = '';
    for (let k = 0; k < data['tiles'].length; k++) {
        loadTile(data['tiles'][k]);
    }

    if (!data.background) {
        // skip for earlier files that only have colors/tiles
        return;
    }

    for (let y = 0; y < BG_HEIGHT_TILES; y++) {
        for (let x = 0; x < BG_WIDTH_TILES; x++) {
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

function savePalettes() {
    const colors = [];
    const outTiles = [];
    let name = document.getElementById('project-name').value;
    if (!name) {
        name = "Untitled";
    }
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
    };
    console.log(JSON.stringify(out));

    const a = document.createElement("a");
    a.href = URL.createObjectURL(new Blob([JSON.stringify(out)], {
        type: 'text/plain'
    }));
    a.setAttribute('download', `${name}.json`);
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
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
    tile.canvas.classList.add('tile-highlighted');
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
    const overlay = document.getElementById('background-overlay');
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

    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            pixels[y][x].setAttribute('data-palette-index', tile.data[y * 16 + x].toString());
        }
    }

    editorOverlay.style.display = 'flex';
    editorPaletteSelector.value = tile.palette.toString();
    editorPaletteSelector.onchange();
}

function createTile() {
    const indices = new Array(256).fill(0);
    const canvas = document.createElement('canvas');
    const tile = {
        data: indices,
        palette: parseInt(editorPaletteSelector.value),
        canvas: canvas,
    };

    canvas.className = 'tile';
    canvas.width = 16;
    canvas.height = 16;
    canvas.style.width = '64px';
    canvas.style.height = '64px';
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
    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            const colorIndex = parseInt(pixels[y][x].getAttribute('data-palette-index'));
            editedTile.data[y * 16 + x] = colorIndex;
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
    const ctx = tile.canvas.getContext("2d");
    const image = ctx.createImageData(16, 16);
    const basePaletteIndex = tile.palette * 16;

    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            const index = 4 * (y * 16 + x);
            const colorIndex = parseInt(tile.data[y * 16 + x]);
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
            if (background[y][x] === -1) {
                bgContext.clearRect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE)
            } else { 
                const tile = tiles[background[y][x]];
                bgContext.drawImage(tile.canvas, x * TILE_SIZE, y * TILE_SIZE);
            }
        }
    }
}

function updateOverlay(tile) {
    const overlay = document.getElementById('background-overlay');
    const ctx = overlay.getContext("2d");

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
    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
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
        for (let k = 0; k < obj.colors.length; k++) {
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

function initializePalettes() {
    for (let pn = 0; pn < 8; pn++) {
        const palette = makePalette(pn);
        document.getElementById('palettes').appendChild(palette);
    }

    const editorPalette = makePalette(null, true);
    document.getElementById('editor-palette-area').appendChild(editorPalette);
}

function initializePixels() {
    for (let y = 0; y < 16; y++) {
        pixels[y] = [];
        const row = document.createElement('div');
        row.className = 'pixel-row';
        for (let x = 0; x < 16; x++) {
            const pix = document.createElement('div');
            pix.className = 'pixel';
            pix.style.backgroundImage = 'url("transparent.png")';
            pix.style.backgroundRepeat = 'repeat';
            pix.setAttribute('data-palette-index', '0');
    
            pix.onmousedown = function(evt) {
                placePixel(pix, evt.shiftKey);
                redrawPixels();
            };
            pix.onmousemove = function(evt) {
                if (evt.buttons) {
                    placePixel(pix, evt.shiftKey);
                    redrawPixels();
                }
            }
            pixels[y].push(pix);
            row.appendChild(pix);
        }
    
        document.getElementById('pixels-container').appendChild(row);
    }
}

function initializeBackground() {
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

    const bgCanvas = document.getElementById('background-canvas');
    bgCanvas.onmousemove = function(e) {
        const overlay = document.getElementById('background-overlay');
        const x = Math.floor(e.offsetX / 32);
        const y = Math.floor(e.offsetY / 32);
        overlay.style.left = `${x * 32}px`;
        overlay.style.top = `${y * 32}px`;

        if (background[y][x] !== -1) {
            highlightTile(tiles[background[y][x]]);
        }

        if (e.buttons) {
            placeTile(x, y, e.shiftKey);
        }
    }

    bgCanvas.onclick = function(e) {
        const x = Math.floor(e.offsetX / 32);
        const y = Math.floor(e.offsetY / 32);

        placeTile(x, y, e.shiftKey);
    }
    
    window.onbeforeunload = function() {
        return true;
    };    
}

initializePalettes();
initializePixels();
initializeBackground();
addEventHandlers();
