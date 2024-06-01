const pixels = [];
const paletteEntries = [];
const editorPaletteEntries = [];

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
        pix.onclick = function() {
            if (this.hasAttribute('data-selected')) {
                this.removeAttribute('data-selected');
                this.style.backgroundColor = '#000000';
            } else {
                this.setAttribute('data-selected', 'true');
                this.style.backgroundColor = '#00ffff';
            }
        };
        pixels[y].push(pix);
        row.appendChild(pix);
    }

    document.getElementById('pixels-container').appendChild(row);
}

function openTileEditor() {
    document.getElementById('tile-editor-overlay').style.display = 'block';
}

for (let pn = 0; pn < 8; pn++) {
    const palette = makePalette(pn);
    document.getElementById('palettes').appendChild(palette);
}

const editorPalette = makePalette(null, true);
document.getElementById('tile-editor').appendChild(editorPalette);