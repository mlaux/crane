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
            const palette = createPalette(pn);
            paletteContainer.appendChild(palette);
        }
    
        const editorPalette = createPalette(null, true);
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
                if (useX >= tileSize || useY >= tileSize) {
                    return;
                }

                const curPix = pixels[useY][useX];

                if (evt.buttons) {
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
    
    document.getElementById('toggle-grid').onclick = function(e) {
        toggleGrid();
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

    document.body.onscroll = e => e.preventDefault();
    
    window.onbeforeunload = function() {
        return true;
    };
}

initialize(DEFAULT_TILE_SIZE);
addEventHandlers();