const pixels = [];
const paletteEntries = [];
const editorPaletteEntries = [];

const tiles = [];
let curTile = null;

const editorOverlay = document.getElementById('tile-editor-overlay');
const editor = document.getElementById('tile-editor');
const editorPaletteSelector = document.getElementById('editor-palette-selector');

function parseData(data) {
    for (let k = 0; k < data['colors'].length; k++) {
        paletteEntries[k].value = data['colors'][k];
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
    for (let k = 0; k < paletteEntries.length; k++) {
        colors[k] = paletteEntries[k].value;
    }
    for (let k = 0; k < tiles.length; k++) {
        outTiles[k] = tiles[k].data;
    }
    const out = { colors, tiles: outTiles, };
    console.log(JSON.stringify(out));

    const a = document.createElement("a");
    a.href = URL.createObjectURL(new Blob([JSON.stringify(out, null, 2)], {
        type: 'text/plain'
    }));
    a.setAttribute('download', "data.json");
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
            editorPaletteEntries.push(entry);
        } else {
            entry.classList.add('palette-entry-global');
            paletteEntries.push(entry);
        }
        row.appendChild(entry);
    }
    return row;
}

for (let y = 0; y < 16; y++) {
    pixels[y] = [];
    const row = document.createElement('div');
    row.className = 'pixel-row';
    for (let x = 0; x < 16; x++) {
        const pix = document.createElement('div');
        pix.className = 'pixel';
        pix.style.backgroundColor = '#000000';
        pix.setAttribute('data-palette-index', '0');

        pix.onclick = function() {
            const selectedEntries = document.getElementsByClassName('palette-entry-selected');
            if (selectedEntries.length) {
                const selectedEntry = selectedEntries[0];
                const index = editorPaletteEntries.indexOf(selectedEntry);
                this.style.backgroundColor = selectedEntries[0].value;
                this.setAttribute('data-palette-index', index.toString());
            }
        };
        pixels[y].push(pix);
        row.appendChild(pix);
    }

    document.getElementById('pixels-container').appendChild(row);
}

function openTileEditor(tile) {
    curTile = tile;

    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            let useValue = 0;
            if (tile) {
                useValue = tile.data[y * 16 + x];
            }
            pixels[y][x].setAttribute('data-palette-index', useValue.toString());
        }
    }

    editorOverlay.style.display = 'flex';
    editorPaletteSelector.onchange();
}

function closeTileEditor() {
    editorOverlay.style.display = 'none';

    let indices;
    let canvas;
    if (curTile) {
        indices = curTile.data;
        canvas = curTile.canvas;
    } else {
        indices = [];
        canvas = document.createElement('canvas');
        const newTile = {
            data: indices,
            canvas: canvas,
        };

        canvas.className = 'tile';
        canvas.width = 16;
        canvas.height = 16;
        canvas.style.backgroundColor = 'black';
        canvas.onclick = function() {
            openTileEditor(newTile);
        };

        document.getElementById('tile-library').appendChild(canvas);
        tiles.push(newTile);
    }

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

function redrawPixels() {
    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            const colorIndex = pixels[y][x].getAttribute('data-palette-index');
            const color = editorPaletteEntries[colorIndex].value;
            pixels[y][x].style.backgroundColor = color;
        }
    }
}

editorOverlay.onclick = function(evt) {
    const els = document.elementsFromPoint(evt.x, evt.y);
    // don't close if clicked inside the editor
    if (els.indexOf(editor) === -1) {
        closeTileEditor();
    }
};

editorPaletteSelector.onchange = function() {
    copyPaletteToEditor(editorPaletteSelector.value);
    redrawPixels();
};

for (let pn = 0; pn < 8; pn++) {
    const palette = makePalette(pn);
    document.getElementById('palettes').appendChild(palette);
}

const editorPalette = makePalette(null, true);
document.getElementById('editor-palette-area').appendChild(editorPalette);