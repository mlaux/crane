// DOM elements of the pixels in the tile editor
const pixels = [];
// color values
const paletteColors = [];
const editorPaletteColors = [];
// color preview elements
const paletteEntries = [];
const editorPaletteEntries = [];

// list of tiles in the tile library
const tiles = [];
let editedTile = null;
let selectedTileIndex = -1;

// 2d array of indices into the tiles array
const background = [];

// for tile editor. TODO allow zooming the BG as well
const ZOOMS = [8, 16, 32];
let zoomIndex = 1;

const COLORS_PER_PALETTE = 16;

const BG_WIDTH_TILES = 32;
const BG_HEIGHT_TILES = 32;

// TODO configurable
const BG_SCALE_FACTOR = 2;

const bgCanvas = document.getElementById('background-canvas');
const overlay = document.getElementById('background-overlay');

const editorOverlay = document.getElementById('tile-editor-overlay');
const editor = document.getElementById('tile-editor');
const editorPaletteSelector = document.getElementById('editor-palette-selector');

const DEFAULT_TILE_SIZE = 16;
let tileSize;
let gridVisible = true;
