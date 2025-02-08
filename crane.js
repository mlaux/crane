const pixels = [];
const paletteEntries = [];
const editorPaletteEntries = [];

const tiles = [];
let curTile = null;

const ZOOMS = [8, 16, 32];
let zoomIndex = 1;

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
}

function loadPalettes() {
    const input = document.createElement('input');
    input.type = 'file';

    input.onchange = function(e) { 
        const file = e.target.files[0]; 
        const reader = new FileReader();
        reader.readAsText(file, 'UTF-8');
        reader.onload = function(readerEvent) {
            parseData(JSON.parse(readerEvent.target.result));
        };
    };

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

function makePalette(text, forTileEditor) {
    const row = document.createElement('div');
    if (text !== null) {
        row.innerText = text + ' ';
    }
    row.style.display = 'inline-block';
    for (let x = 0; x < 16; x++) {
        const entry = document.createElement('input');
        entry.type = 'color';
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
            entry.onchange = function() {
                copyEditorPaletteToGlobal(editorPaletteSelector.value);
                redrawTiles();
                redrawPixels();
            }
            editorPaletteEntries.push(entry);
        } else {
            entry.classList.add('palette-entry-global');
            entry.onchange = redrawTiles;
            paletteEntries.push(entry);
        }
        if (x != 0) {
            row.appendChild(entry);
        }
    }
    return row;
}

function openTileEditor(tile) {
    if (!tile) {
        tile = createTile();
    }
    curTile = tile;

    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            pixels[y][x].setAttribute('data-palette-index', tile.data[y * 16 + x].toString());
        }
    }

    editorOverlay.style.display = 'flex';
    editorPaletteSelector.value = tile.palette;
    editorPaletteSelector.onchange();
}

function createTile() {
    const indices = new Array(256).fill(0);
    const canvas = document.createElement('canvas');
    const newTile = {
        data: indices,
        palette: editorPaletteSelector.value,
        canvas: canvas,
    };

    canvas.className = 'tile';
    canvas.width = 16;
    canvas.height = 16;
    canvas.style.width = '64px';
    canvas.style.height = '64px';
    canvas.style.backgroundColor = 'black';
    canvas.onclick = function() {
        openTileEditor(newTile);
    };

    document.getElementById('tile-entries').appendChild(canvas);
    tiles.push(newTile);
    return newTile;
}

function closeTileEditor() {
    editorOverlay.style.display = 'none';

    let indices = curTile.data;
    let canvas = curTile.canvas;

    const ctx = canvas.getContext("2d");
    const image = ctx.createImageData(16, 16);

    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            const index = 4 * (y * 16 + x);
            const colorIndex = parseInt(pixels[y][x].getAttribute('data-palette-index'));
            const color = parseInt(editorPaletteEntries[colorIndex].value.substring(1), 16);
            image.data[index] = color >> 16 & 0xff;
            image.data[index + 1] = color >> 8 & 0xff;
            image.data[index + 2] = color & 0xff;
            image.data[index + 3] = 0xff;
            indices[y * 16 + x] = colorIndex;
        }
    }
    ctx.putImageData(image, 0, 0);
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
            const color = parseInt(paletteEntries[basePaletteIndex + colorIndex].value.substring(1), 16);
            image.data[index] = color >> 16 & 0xff;
            image.data[index + 1] = color >> 8 & 0xff;
            image.data[index + 2] = color & 0xff;
            image.data[index + 3] = 0xff;
        }
    }
    ctx.putImageData(image, 0, 0);
}

function redrawTiles() {
    tiles.forEach(redrawTile);
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

        pix.onclick = function(evt) {
            if (evt.shiftKey) {
                this.setAttribute('data-palette-index', '0');
            } else {
                const selectedEntries = document.getElementsByClassName('palette-entry-selected');
                if (selectedEntries.length) {
                    const selectedEntry = selectedEntries[0];
                    const index = editorPaletteEntries.indexOf(selectedEntry);
                    this.setAttribute('data-palette-index', index.toString());
                }
            }
            redrawPixels();
        };
        pixels[y].push(pix);
        row.appendChild(pix);
    }

    document.getElementById('pixels-container').appendChild(row);
}

editorOverlay.onclick = function(evt) {
    const els = document.elementsFromPoint(evt.x, evt.y);
    // don't close if clicked inside the editor
    if (els.indexOf(editor) === -1) {
        closeTileEditor();
    }
};

editorPaletteSelector.onchange = function() {
    curTile.palette = editorPaletteSelector.value;
    copyPaletteToEditor(editorPaletteSelector.value);
    redrawPixels();
};

for (let pn = 0; pn < 8; pn++) {
    const palette = makePalette(pn);
    document.getElementById('palettes').appendChild(palette);
}

const editorPalette = makePalette(null, true);
document.getElementById('editor-palette-area').appendChild(editorPalette);

document.getElementById('zoom-in').onclick = function(e) {
    zoomIn();
    e.stopPropagation();
}

document.getElementById('zoom-out').onclick = function(e) {
    zoomOut();
    e.stopPropagation();
}

window.onbeforeunload = function() {
    return true;
};