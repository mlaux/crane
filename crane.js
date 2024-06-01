const pixels = [];
const paletteEntries = [];
const editorPaletteEntries = [];

const editorOverlay = document.getElementById('tile-editor-overlay');
const editor = document.getElementById('tile-editor');
const editorPaletteSelector = document.getElementById('editor-palette-selector');

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

function openTileEditor() {
    editorOverlay.style.display = 'flex';
    editorPaletteSelector.onchange();
}

function closeTileEditor() {
    editorOverlay.style.display = 'none';
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
    redrawPixels(editorPaletteSelector.value);
};

for (let pn = 0; pn < 8; pn++) {
    const palette = makePalette(pn);
    document.getElementById('palettes').appendChild(palette);
}

const editorPalette = makePalette(null, true);
document.getElementById('editor-palette-area').appendChild(editorPalette);