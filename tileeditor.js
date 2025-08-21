
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

function closeTileEditor() {
    editorOverlay.style.display = 'none';

    // copy from pixel editor to tile data
    for (let y = 0; y < tileSize; y++) {
        for (let x = 0; x < tileSize; x++) {
            const colorIndex = parseInt(pixels[y][x].getAttribute('data-palette-index'));
            editedTile.data[y * tileSize + x] = colorIndex;
        }
    }

    getColorPicker().hide(false);

    // editing a tile affects both the tile and the background
    redrawTile(editedTile);
    redrawBackground();
    // don't update overlay if editing a tile that's not the selected one
    if (selectedTileIndex !== -1 && editedTile === tiles[selectedTileIndex]) {
        updateOverlay(editedTile);
    }
    editedTile = null;
}

function copyPaletteToEditor(index) {
    const base = index * 16;
    for (let k = 0; k < 16; k++) {
        editorPaletteColors[k] = paletteColors[base + k];
    }
    updatePaletteUI();
}

function selectPaletteEntry(entry) {
    for (let k = 0; k < editorPaletteEntries.length; k++) {
        editorPaletteEntries[k].classList.remove('palette-entry-selected');
    }
    entry.classList.add('palette-entry-selected');
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

function toggleGrid() {
    gridVisible = !gridVisible;
    const pixelsContainer = document.getElementById('pixels-container');
    const toggleButton = document.getElementById('toggle-grid');
    
    if (gridVisible) {
        pixelsContainer.classList.remove('no-grid');
        toggleButton.textContent = 'Hide grid';
    } else {
        pixelsContainer.classList.add('no-grid');
        toggleButton.textContent = 'Show grid';
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
                const color = editorPaletteColors[colorIndex];
                pixels[y][x].style.backgroundImage = '';
                pixels[y][x].style.backgroundColor = color;
            }
        }
    }
}
