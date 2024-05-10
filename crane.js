const pixels = [];

function makePalette(pn) {
    const row = document.createElement('div');
    row.className = 'pixel-row';
    row.innerText = pn + ' ';
    for (let x = 0; x < 16; x++) {
        const entry = document.createElement('input');
        entry.type = 'color';
        entry.className = 'palette-entry';
        //entry.className = 'pixel';

        row.appendChild(entry);
    }
    document.getElementById('palettes').appendChild(row);
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
            //recomputeArray();
        };
        pixels[y].push(pix);
        row.appendChild(pix);
    }

    document.getElementById('pixels-container').appendChild(row);
}

for (let pn = 0; pn < 8; pn++) {
    makePalette(pn);
}