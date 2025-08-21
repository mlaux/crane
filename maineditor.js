// empty means no tiles, all blank background, all black palettes
function isDocumentEmpty() {
    return !tiles.length 
            && !background.some(row => row.some(el => el != -1))
            && !paletteColors.some(color => color != '#000000');
}

function createPalette(text, forTileEditor) {
    const row = document.createElement('div');
    row.classList.add('palette-row');
    row.style.display = 'inline-block';
    if (text !== null) {
        row.innerText = text + ' ';
    }
    for (let x = 0; x < 16; x++) {
        const entry = document.createElement('div');
        entry.classList.add('palette-entry');
        
        if (forTileEditor) {
            entry.classList.add('palette-entry-editor');
            entry.onclick = function(evt) {
                evt.preventDefault();
                if (evt.shiftKey) {
                    const colorIndex = editorPaletteEntries.indexOf(entry);
                    showColorPicker(editorPaletteColors[colorIndex], (newColor) => {
                        editorPaletteColors[colorIndex] = newColor;
                        onTileEditorPaletteEntryChanged();
                    }, entry);
                } else {
                    selectPaletteEntry(entry);
                }
            };
            editorPaletteEntries.push(entry);
        } else {
            entry.classList.add('palette-entry-global');
            entry.onclick = function(evt) {
                const globalIndex = paletteEntries.indexOf(entry);

                showColorPicker(paletteColors[globalIndex], (newColor) => {
                    paletteColors[globalIndex] = newColor;
                    onGlobalPaletteEntryChanged();
                }, entry);
            };
            paletteEntries.push(entry);
        }
        if (x != 0) {
            // skip element for transparent color index 0
            row.appendChild(entry);
        }
    }
    return row;
}

function showColorPicker(currentColor, callback, anchor = null) {
    const picker = getColorPicker();
    picker.show(currentColor, callback, anchor);
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

function placeTile(x, y, remove) {
    background[y][x] = remove ? -1 : selectedTileIndex;
    redrawBackground();
}

function duplicateTile() {
    if (selectedTileIndex === -1) {
        return;
    }

    const originalTile = tiles[selectedTileIndex];
    const duplicatedTile = createTile();
    
    // Copy the data and palette from the original tile
    duplicatedTile.data = [...originalTile.data];
    duplicatedTile.palette = originalTile.palette;
    
    // Redraw the duplicated tile to show the copied pixels
    redrawTile(duplicatedTile);
    
    // Select the newly created duplicate
    selectTile(duplicatedTile);
}

function onGlobalPaletteEntryChanged() {
    updatePaletteUI();
    redrawTiles();
    redrawBackground();
}

function onTileEditorPaletteEntryChanged() {
    copyEditorPaletteToGlobal(parseInt(editorPaletteSelector.value));
    redrawPixels();
    redrawTiles();
    redrawBackground();
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
                const color = parseInt(paletteColors[basePaletteIndex + colorIndex].substring(1), 16);

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

function copyEditorPaletteToGlobal(index) {
    const base = index * 16;
    for (let k = 0; k < 16; k++) {
        paletteColors[base + k] = editorPaletteColors[k];
    }
    updatePaletteUI();
}

// import a palette from https://lospec.com/palette-list/${name}.json
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
            paletteColors[baseIndex + k] = clampHexStringToRGB555Hex(obj.colors[k]);
        }
        updatePaletteUI();

        urlField.value = '';

        redrawTiles();
        redrawBackground();
        if (selectedTileIndex !== -1) {
            updateOverlay(tiles[selectedTileIndex]);
        }
    })();
}

function updatePaletteUI() {
    for (let k = 0; k < paletteEntries.length; k++) {
        paletteEntries[k].style.backgroundColor = paletteColors[k];
    }

    for (let k = 0; k < editorPaletteEntries.length; k++) {
        editorPaletteEntries[k].style.backgroundColor = editorPaletteColors[k];
    }
}