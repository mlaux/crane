function clampHexStringToRGB555Hex(color) {
    const hex = color.replace('#', '');
    const r = parseInt(hex.substr(0, 2), 16);
    const g = parseInt(hex.substr(2, 2), 16);
    const b = parseInt(hex.substr(4, 2), 16);
    const [clampedR, clampedG, clampedB] = clampTo555(r, g, b);
    return `#${clampedR.toString(16).padStart(2, '0')}${clampedG.toString(16).padStart(2, '0')}${clampedB.toString(16).padStart(2, '0')}`;
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
        const color = data['colors'][k];
        paletteColors[k] = clampHexStringToRGB555Hex(color);
    }
    updatePaletteUI();

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

function loadProject() {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';
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

function loadTile(data) {
    const tile = createTile();
    tile.data = data.data;
    tile.palette = data.palette;
    redrawTile(tile);
}

function saveProject() {
    const colors = [];
    const outTiles = [];
    const name = getProjectName();
    for (let k = 0; k < paletteColors.length; k++) {
        colors[k] = paletteColors[k];
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

function saveSinglePalette() {
    const paletteIndex = parseInt(document.getElementById('palette-loadsave-index').value);
    const colors = [];
    const baseIndex = paletteIndex * COLORS_PER_PALETTE;
    
    for (let k = 0; k < COLORS_PER_PALETTE; k++) {
        colors[k] = paletteColors[baseIndex + k];
    }

    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([JSON.stringify(colors)], {
        type: 'text/plain'
    }));
    a.setAttribute('download', `palette_${paletteIndex}.json`);
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
}

function loadSinglePalette() {
    const paletteIndex = parseInt(document.getElementById('palette-loadsave-index').value);

    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';
    document.body.appendChild(input);

    input.addEventListener('change', evt => { 
        const file = evt.target.files[0]; 
        const reader = new FileReader();
        reader.readAsText(file, 'UTF-8');
        reader.onload = readerEvent => {
            try {
                const colors = JSON.parse(readerEvent.target.result);
                if (!Array.isArray(colors)) {
                    alert('Invalid palette file: must be a JSON array of colors');
                    return;
                }

                const baseIndex = paletteIndex * COLORS_PER_PALETTE;
                for (let k = 0; k < COLORS_PER_PALETTE && k < colors.length; k++) {
                    const color = colors[k];
                    paletteColors[baseIndex + k] = clampHexStringToRGB555Hex(color);
                }
                updatePaletteUI();

                redrawTiles();
                redrawBackground();
                if (selectedTileIndex !== -1) {
                    updateOverlay(tiles[selectedTileIndex]);
                }
            } catch (e) {
                alert('Invalid JSON file');
            }
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

function importReferenceImage() {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = 'image/*';
    document.body.appendChild(input);

    input.addEventListener('change', evt => {
        const file = evt.target.files[0];
        if (!file) {
            return;
        }
        const reader = new FileReader();
        reader.readAsDataURL(file);
        reader.onload = readerEvent => {
            bgUnderlay.style.backgroundImage = `url(${readerEvent.target.result})`;
        };
        reader.onloadend = () => {
            document.body.removeChild(input);
        };
    });

    input.click();
}