function clampTo555(r, g, b) {
    const r5 = r >> 3;
    const g5 = g >> 3;
    const b5 = b >> 3;

    return [r5 << 3, g5 << 3, b5 << 3];
}

function hslToRgb(h, s, l) {
    // Convert HSL to RGB (all values 0-1)
    h = h / 360;
    s = s / 100;
    l = l / 100;
    
    let r, g, b;
    
    if (s === 0) {
        r = g = b = l;
    } else {
        const hue2rgb = (p, q, t) => {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1/6) return p + (q - p) * 6 * t;
            if (t < 1/2) return q;
            if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
            return p;
        };
        
        const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        const p = 2 * l - q;
        r = hue2rgb(p, q, h + 1/3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1/3);
    }
    
    return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

function rgbToHsl(r, g, b) {
    r /= 255;
    g /= 255;
    b /= 255;
    
    const max = Math.max(r, g, b);
    const min = Math.min(r, g, b);
    let h, s, l = (max + min) / 2;
    
    if (max === min) {
        h = s = 0;
    } else {
        const d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        
        switch (max) {
            case r: h = (g - b) / d + (g < b ? 6 : 0); break;
            case g: h = (b - r) / d + 2; break;
            case b: h = (r - g) / d + 4; break;
        }
        h /= 6;
    }
    
    return [h * 360, s * 100, l * 100];
}

const ALL_EVENTS = [
    'mousedown', 
    'mouseup', 
    'mousemove', 
    'mouseover', 
    'mouseout', 
    'mouseenter', 
    'mouseleave', 
    'contextmenu', 
    'keydown', 
    'keyup', 
    'keypress'
];

class RGB555ColorPicker {
    constructor() {
        this.currentH = 0;
        this.currentS = 100;
        this.currentL = 50;
        this.isDraggingHS = false;
        this.isDraggingLightness = false;
        this.onColorChange = null;
        this.container = null;
        this.isSetFromMode = false;
        this.blockAllEvents = evt => {
            if (!this.container.contains(evt.target)) {
                evt.preventDefault();
                evt.stopPropagation();
                evt.stopImmediatePropagation();
                return false;
            }
        };
    
        this.createPickerElements();
        this.setupEventHandlers();
    }
    
    createPickerElements() {
        this.container = document.createElement('div');
        this.container.className = 'color-picker';
        this.container.style.cssText = `
            position: fixed;
            top: 50%;
            right: 20px;
            transform: translateY(-50%);
            display: none;
            background: #282828;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            padding: 16px;
            z-index: 1000;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        `;
        
        // Create HS rectangle container
        const hsContainer = document.createElement('div');
        hsContainer.className = 'hs-container';
        hsContainer.style.cssText = `
            position: relative;
            margin-bottom: 8px;
            line-height: 0;
        `;
        
        // Create HS canvas
        this.hsCanvas = document.createElement('canvas');
        this.hsCanvas.className = 'hs-rectangle';
        this.hsCanvas.width = 256;
        this.hsCanvas.height = 256;
        this.hsCanvas.style.cssText = `
            width: 256px;
            height: 256px;
            margin: 0;
            cursor: crosshair;
        `;
        this.hsCtx = this.hsCanvas.getContext('2d');
        
        // Create HS cursor
        this.hsCursor = document.createElement('div');
        this.hsCursor.className = 'hs-cursor';
        this.hsCursor.style.cssText = `
            position: absolute;
            width: 8px;
            height: 8px;
            border: 2px solid white;
            border-radius: 50%;
            pointer-events: none;
            transform: translate(-6px, -6px);
            box-shadow: 0 0 0 1px black;
        `;
        
        hsContainer.appendChild(this.hsCanvas);
        hsContainer.appendChild(this.hsCursor);
        
        // Create lightness slider container
        const lightnessContainer = document.createElement('div');
        lightnessContainer.className = 'lightness-container';
        lightnessContainer.style.cssText = `
            position: relative;
            margin-bottom: 8px;
        `;
        
        // Create lightness canvas
        this.lightnessSlider = document.createElement('canvas');
        this.lightnessSlider.className = 'lightness-slider';
        this.lightnessSlider.width = 256;
        this.lightnessSlider.height = 20;
        this.lightnessSlider.style.cssText = `
            width: 256px;
            height: 20px;
            cursor: pointer;
        `;
        this.lightnessCtx = this.lightnessSlider.getContext('2d');
        
        // Create lightness cursor
        this.lightnessCursor = document.createElement('div');
        this.lightnessCursor.className = 'lightness-cursor';
        this.lightnessCursor.style.cssText = `
            position: absolute;
            width: 2px;
            height: 22px;
            top: -2px;
            background: white;
            border: 1px solid black;
            pointer-events: none;
            transform: translateX(-1px);
        `;
        
        lightnessContainer.appendChild(this.lightnessSlider);
        lightnessContainer.appendChild(this.lightnessCursor);
        
        // Create color preview
        this.colorPreview = document.createElement('div');
        this.colorPreview.className = 'color-preview';
        this.colorPreview.style.cssText = `
            height: 32px;
            border: 1px solid #000;
            margin-bottom: 8px;
            box-sizing: border-box;
        `;
        
        // Create hex input
        this.colorText = document.createElement('input');
        this.colorText.type = 'text';
        this.colorText.value = '#000000';
        this.colorText.style.cssText = `
            width: 80px;
            font-family: monospace;
            margin-bottom: 8px;
        `;
        
        // Create info display
        this.infoDisplay = document.createElement('div');
        this.infoDisplay.style.cssText = `
            font-family: monospace;
            font-size: 12px;
            color: #ccc;
            margin-bottom: 8px;
            line-height: 1.4;
        `;
        
        // Create buttons
        const buttonContainer = document.createElement('div');
        buttonContainer.style.cssText = 'text-align: center;';
        
        this.setFromButton = document.createElement('button');
        this.setFromButton.style.marginRight = '4px';
        this.setFromButton.textContent = 'Set from';
        this.setFromButton.onclick = () => this.toggleSetFromMode();
        
        const okButton = document.createElement('button');
        okButton.style.marginRight = '4px';
        okButton.textContent = 'OK';
        okButton.onclick = () => this.hide(true);
        
        const cancelButton = document.createElement('button');
        cancelButton.textContent = 'Cancel';
        cancelButton.onclick = () => this.hide(false);
        
        buttonContainer.appendChild(this.setFromButton);
        buttonContainer.appendChild(okButton);
        buttonContainer.appendChild(cancelButton);
        
        // Assemble picker
        this.container.appendChild(hsContainer);
        this.container.appendChild(lightnessContainer);
        this.container.appendChild(this.colorPreview);
        this.container.appendChild(this.colorText);
        this.container.appendChild(this.infoDisplay);
        this.container.appendChild(buttonContainer);
        
        document.body.appendChild(this.container);
    }
    
    setupEventHandlers() {
        // HS rectangle events
        this.hsCanvas.addEventListener('mousedown', (e) => {
            this.isDraggingHS = true;
            this.updateHSFromEvent(e);
            e.preventDefault();
        });

        // Lightness slider events
        this.lightnessSlider.addEventListener('mousedown', (e) => {
            this.isDraggingLightness = true;
            this.updateLightnessFromEvent(e);
            e.preventDefault();
        });
        
        // Global mouse events for dragging
        document.addEventListener('mousemove', (e) => {
            if (this.isDraggingHS) {
                this.updateHSFromEvent(e);
            } else if (this.isDraggingLightness) {
                this.updateLightnessFromEvent(e);
            }
        });
        
        document.addEventListener('mouseup', () => {
            this.isDraggingHS = false;
            this.isDraggingLightness = false;
        });
        
        // Global click listener for set from mode
        document.addEventListener('click', (e) => {
            if (this.isSetFromMode && !this.container.contains(e.target)) {
                e.preventDefault();
                e.stopPropagation();
                const color = this.getColorAtScreenPoint(e.clientX, e.clientY);
                if (color) {
                    this.setFromColor(color);
                }
            }
        }, true);
        
        // Hex input events
        this.colorText.addEventListener('input', (e) => {
            this.setColorFromHex(e.target.value);
        });
        
        this.colorText.addEventListener('change', (e) => {
            this.setColorFromHex(e.target.value);
        });
    }
    
    drawHSRectangle() {
        const imageData = this.hsCtx.createImageData(256, 256);
        const data = imageData.data;
        
        for (let y = 0; y < 256; y++) {
            for (let x = 0; x < 256; x++) {
                const h = (x / 256) * 360;
                const s = 100 - (y / 256) * 100; // y=0 is fully saturated
                
                let [r, g, b] = hslToRgb(h, s, this.currentL);
                [r, g, b] = clampTo555(r, g, b);

                const index = (y * 256 + x) * 4;
                data[index] = r;     // Red
                data[index + 1] = g; // Green
                data[index + 2] = b; // Blue
                data[index + 3] = 255; // Alpha
            }
        }
        
        this.hsCtx.putImageData(imageData, 0, 0);
    }
    
    drawLightnessSlider() {
        const imageData = this.lightnessCtx.createImageData(256, 20);
        const data = imageData.data;
        
        for (let x = 0; x < 256; x++) {
            const l = (x / 256) * 100;
            
            // Convert HSL to RGB
            let [r, g, b] = hslToRgb(this.currentH, this.currentS, l);
            
            // Convert to RGB555 and back to ensure valid SNES colors
            [r, g, b] = clampTo555(r, g, b);
            
            for (let y = 0; y < 20; y++) {
                const index = (y * 256 + x) * 4;
                data[index] = r;
                data[index + 1] = g;
                data[index + 2] = b;
                data[index + 3] = 255;
            }
        }
        
        this.lightnessCtx.putImageData(imageData, 0, 0);
    }
    
    updateUI() {
        let [r, g, b] = hslToRgb(this.currentH, this.currentS, this.currentL);
        [r, g, b] = clampTo555(r, g, b);
        
        // Update color preview
        const hex = `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`;
        this.colorPreview.style.backgroundColor = hex;
        this.colorText.value = hex.toUpperCase();
        
        // Update info display
        const r5 = Math.floor(r / 8);
        const g5 = Math.floor(g / 8);
        const b5 = Math.floor(b / 8);
        const rgb555 = (r5 << 0) | (g5 << 5) | (b5 << 10);
        
        this.infoDisplay.innerHTML = `
            HSL: ${Math.round(this.currentH)}°, ${Math.round(this.currentS)}%, ${Math.round(this.currentL)}%<br>
            RGB: ${r}, ${g}, ${b}<br>
            RGB555: $${rgb555.toString(16).padStart(4, '0')}
        `;
        
        // Update cursor positions
        const hsX = (this.currentH / 360) * 256;
        const hsY = ((100 - this.currentS) / 100) * 256;
        this.hsCursor.style.left = hsX + 'px';
        this.hsCursor.style.top = hsY + 'px';
        
        const lightnessX = (this.currentL / 100) * 256;
        this.lightnessCursor.style.left = lightnessX + 'px';
        
        // Redraw canvases
        this.drawHSRectangle();
        this.drawLightnessSlider();
    }
    
    setColorFromHex(hexString) {
        // Parse hex color
        const hex = hexString.replace('#', '');
        if (hex.length !== 6 || !/^[0-9A-Fa-f]{6}$/.test(hex)) {
            return false; // Invalid hex
        }
        
        let r = parseInt(hex.substr(0, 2), 16);
        let g = parseInt(hex.substr(2, 2), 16);
        let b = parseInt(hex.substr(4, 2), 16);
        [r, g, b] = clampTo555(r, g, b);

        const [h, s, l] = rgbToHsl(r, g, b);
        
        this.currentH = h;
        this.currentS = s;
        this.currentL = l;
        
        this.updateUI();
        return true;
    }
    
    updateHSFromEvent(e) {
        const rect = this.hsCanvas.getBoundingClientRect();
        const x = Math.max(0, Math.min(256, e.clientX - rect.left));
        const y = Math.max(0, Math.min(256, e.clientY - rect.top));
        
        this.currentH = (x / 256) * 360;
        this.currentS = 100 - (y / 256) * 100;
        
        this.updateUI();
    }
    
    updateLightnessFromEvent(e) {
        const rect = this.lightnessSlider.getBoundingClientRect();
        const x = Math.max(0, Math.min(256, e.clientX - rect.left));
        
        this.currentL = (x / 256) * 100;
        
        this.updateUI();
    }
    
    show(initialColor, callback, anchor = null) {
        this.onColorChange = callback;
        this.setColorFromHex(initialColor);
        this.positionNearAnchor(anchor);
        this.container.style.display = 'block';
    }
    
    positionNearAnchor(anchor) {
        if (!anchor) {
            this.container.style.top = '50%';
            this.container.style.right = '20px';
            this.container.style.left = 'auto';
            this.container.style.transform = 'translateY(-50%)';
            return;
        }
        
        const rect = anchor.getBoundingClientRect();
        const pickerWidth = 300;
        const pickerHeight = 500;
        
        // Try to position to the right of the anchor
        let left = rect.right + 16;
        let top = rect.top;
        
        // If it would go off the right edge, position to the left
        if (left + pickerWidth > window.innerWidth) {
            left = rect.left - pickerWidth - 16;
        }
        
        // If it would go off the left edge, center horizontally
        if (left < 16) {
            left = (window.innerWidth - pickerWidth) / 2;
        }
        
        // If it would go off the bottom, adjust upward
        if (top + pickerHeight > window.innerHeight) {
            top = window.innerHeight - pickerHeight - 16;
        }
        
        // If it would go off the top, adjust downward
        if (top < 16) {
            top = 16;
        }
        
        this.container.style.left = left + 'px';
        this.container.style.top = top + 'px';
        this.container.style.right = 'auto';
        this.container.style.transform = 'none';
    }
    
    hide(accepted) {
        this.container.style.display = 'none';
        if (this.isSetFromMode) {
            this.toggleSetFromMode();
        }
        if (accepted && this.onColorChange) {
            const hex = this.colorText.value;
            this.onColorChange(hex);
        }
        this.onColorChange = null;
    }
    
    getCurrentColor() {
        return this.colorText.value;
    }
    
    toggleSetFromMode() {
        this.isSetFromMode = !this.isSetFromMode;
        this.setFromButton.textContent = this.isSetFromMode ? 'Cancel set from' : 'Set from';
        this.setFromButton.style.backgroundColor = this.isSetFromMode ? '#ff6b6b' : '';
        
        if (this.isSetFromMode) {
            this.enableSetFromMode();
        } else {
            this.disableSetFromMode();
        }
    }

    enableSetFromMode() {
        // Create CSS rule to override all cursors
        this.cursorStyle = document.createElement('style');
        this.cursorStyle.textContent = '* { cursor: crosshair !important; }';
        document.head.appendChild(this.cursorStyle);
        
        // Add event blockers for all interaction events
        ALL_EVENTS.forEach(eventType => {
            document.addEventListener(eventType, this.blockAllEvents, true);
        });
    }

    disableSetFromMode() {
        if (this.cursorStyle) {
            document.head.removeChild(this.cursorStyle);
            this.cursorStyle = null;
        }

        ALL_EVENTS.forEach(eventType => {
            document.removeEventListener(eventType, this.blockAllEvents, true);
        });
    }
    
    setFromColor(color) {
        if (this.isSetFromMode) {
            this.setColorFromHex(color);
            this.toggleSetFromMode();
        }
    }

    getColorAtScreenPoint(clientX, clientY) {
        // temporarily disable pointer-events so that it can get the element 
        // below the overlay
        const overlay = document.getElementById('tile-editor-overlay');
        const originalPointerEvents = overlay.style.pointerEvents;
        overlay.style.pointerEvents = 'none';

        const element = document.elementFromPoint(clientX, clientY);
    
        overlay.style.pointerEvents = originalPointerEvents;
        
        if (!element) {
            return null;
        }
        return this.getColorAtPoint(element, clientX, clientY);
    }
    
    getColorAtPoint(element, clientX, clientY) {
        if (element.tagName === 'CANVAS') {
            return this.getCanvasColorAtPoint(element, clientX, clientY);
        }

        return this.getComputedColor(element);
    }

    getCanvasColorAtPoint(canvas, clientX, clientY) {
        try {
            const rect = canvas.getBoundingClientRect();
            const x = Math.floor((clientX - rect.left) * (canvas.width / rect.width));
            const y = Math.floor((clientY - rect.top) * (canvas.height / rect.height));
            
            if (x < 0 || x >= canvas.width || y < 0 || y >= canvas.height) {
                return null;
            }
            
            const ctx = canvas.getContext('2d');
            const imageData = ctx.getImageData(x, y, 1, 1);
            const [r, g, b] = imageData.data;
            
            return `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`;
        } catch (e) {
            return null;
        }
    }

    getComputedColor(element) {
        try {
            const computed = window.getComputedStyle(element);
            const bgColor = computed.backgroundColor;
            
            // ignore transparent so we can give the .color a try
            if (bgColor && bgColor !== 'rgba(0, 0, 0, 0)' && bgColor !== 'transparent') {
                return this.parseColorWithCanvas(bgColor);
            }
            
            const color = computed.color;
            if (color && color !== 'rgba(0, 0, 0, 0)' && color !== 'transparent') {
                return this.parseColorWithCanvas(color);
            }
            
            return null;
        } catch (e) {
            console.log('error retrieving color', e);
            return null;
        }
    }

    parseColorWithCanvas(colorString) {
        // Use canvas to parse any CSS color format
        const canvas = document.createElement('canvas');
        canvas.width = 1;
        canvas.height = 1;
        const ctx = canvas.getContext('2d');
        
        ctx.fillStyle = colorString;
        ctx.fillRect(0, 0, 1, 1);
        
        const [r, g, b] = ctx.getImageData(0, 0, 1, 1).data;
        return `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`;
    }
}

let colorPicker = null;

function getColorPicker() {
    if (!colorPicker) {
        colorPicker = new RGB555ColorPicker();
    }
    return colorPicker;
}