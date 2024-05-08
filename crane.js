const pixels = [];

for (let y = 0; y < 16; y++) {
    pixels[y] = [];
    for (let x = 0; x < 16; x++) {
        const pix = document.createElement('div');
        pix.style.position = 'absolute';
        pix.style.top = 50 + y * 32 + 'px';
        pix.style.left = 20 + x * 32 + 'px';
        pix.style.width = '32px';
        pix.style.height = '32px';
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
        document.body.appendChild(pix);
        pixels[y].push(pix);
    }
}